#include "DialerInternal.h"

#include "config/Config.h"
#include "net/NetClient.h"
#include "net/NetClientInternal.h"
#include "utils/log/Logger.h"
#include "utils/text/TextUtils.h"
#include "utils/time/TimeUtils.h"

#include <cJSON/cJSON.h>
#include <ctype.h>
#include <curl/curl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sys/stat.h>
#endif

/** 默认认证服务器端口 */
#define AUTH_SERVER_PORT 7001

/** 单台服务器的测速上限: 连不上就算了, 不能拖住整个认证流程 */
#define AUTH_SERVER_TIMEOUT_MS 2000L

/** 单台失败服务器的冷却时间 (毫秒) */
#define AUTH_SERVER_COOLDOWN_MS 120000ULL

/** 文件热更新的最小轮询间隔 (毫秒) */
#define AUTH_RELOAD_CHECK_INTERVAL_MS 500ULL

static const char* const s_default_auth_servers[] = {
    "14.146.227.141:7001",
    "121.8.177.212:7001",
};

#define DEFAULT_AUTH_SERVER_COUNT (sizeof(s_default_auth_servers) / sizeof(s_default_auth_servers[0]))

typedef struct
{
    char host_port[AUTH_SERVER_LEN];
    uint64_t failed_at_ms;
} dead_server_entry_t;

typedef struct
{
    bool valid;
    uint64_t mtime_val;
    uint64_t size_val;
    uint64_t last_check_ms;
} cfg_file_sig_t;

#ifdef _WIN32
static SRWLOCK s_auth_srv_lock = SRWLOCK_INIT;
#define AUTH_SRV_LOCK() AcquireSRWLockExclusive(&s_auth_srv_lock)
#define AUTH_SRV_UNLOCK() ReleaseSRWLockExclusive(&s_auth_srv_lock)
#else
static pthread_mutex_t s_auth_srv_lock = PTHREAD_MUTEX_INITIALIZER;
#define AUTH_SRV_LOCK() pthread_mutex_lock(&s_auth_srv_lock)
#define AUTH_SRV_UNLOCK() pthread_mutex_unlock(&s_auth_srv_lock)
#endif

static char s_dynamic_servers[AUTH_CANDIDATE_MAX][AUTH_SERVER_LEN];
static uint8_t s_dynamic_server_count = 0;

static dead_server_entry_t s_dead_servers[AUTH_CANDIDATE_MAX];
static uint8_t s_dead_server_count = 0;

static cfg_file_sig_t s_cfg_sig = {0};

/** @brief 判断主机名部分是否全是 IPv4 字面量 */
static bool is_ipv4_literal(const char* s)
{
    if (s == NULL || *s == '\0') return false;

    uint8_t parts = 0;
    while (*s != '\0')
    {
        if (isdigit((unsigned char)*s) == 0) return false;

        int value = 0;
        while (isdigit((unsigned char)*s) != 0)
        {
            value = value * 10 + (*s - '0');
            if (value > 255) return false;
            s++;
        }

        parts++;
        if (*s == '\0') break;
        if (*s != '.') return false;
        s++;
    }
    return parts == 4;
}

/** @brief 从 "host:port" 里拆出 host 部分 */
static void hostport_extract_host(const char* host_port, char* out, const size_t out_len)
{
    if (out == NULL || out_len == 0) return;
    out[0] = '\0';
    if (host_port == NULL) return;

    const char* colon = strchr(host_port, ':');
    const size_t len = colon != NULL ? (size_t)(colon - host_port) : strlen(host_port);
    if (len == 0 || len >= out_len) return;

    memcpy(out, host_port, len);
    out[len] = '\0';
}

bool auth_server_normalize(const char* raw, char* out, const size_t out_len)
{
    if (raw == NULL || out == NULL || out_len == 0) return false;
    out[0] = '\0';

    const char* p = raw;
    while (*p != '\0' && isspace((unsigned char)*p)) p++;
    if (*p == '\0') return false;

    if (str_iprefix(p, "https://")) return false;
    if (str_iprefix(p, "http://")) p += 7;

    /* 拒绝其它带 scheme 的地址 (ftp:// 之类) */
    const char* scheme_sep = strstr(p, "://");
    const char* first_slash = strchr(p, '/');
    if (scheme_sep != NULL && (first_slash == NULL || scheme_sep < first_slash)) return false;

    size_t auth_len = strcspn(p, "/?#");
    while (auth_len > 0 && isspace((unsigned char)p[auth_len - 1])) auth_len--;
    if (auth_len == 0 || auth_len >= AUTH_SERVER_LEN) return false;

    char auth_buf[AUTH_SERVER_LEN];
    memcpy(auth_buf, p, auth_len);
    auth_buf[auth_len] = '\0';

    /* authority 段内绝不能出现 userinfo 或控制字符/空白 */
    for (size_t i = 0; i < auth_len; i++)
    {
        const unsigned char ch = (unsigned char)auth_buf[i];
        if (ch <= 0x20 || ch == '@' || ch == '\\') return false;
    }

    char* colon = strchr(auth_buf, ':');
    unsigned port = AUTH_SERVER_PORT;
    if (colon != NULL)
    {
        if (strchr(colon + 1, ':') != NULL) return false;
        *colon = '\0';
        const char* port_str = colon + 1;
        if (*port_str == '\0') return false;

        unsigned parsed_port = 0;
        for (const char* q = port_str; *q != '\0'; q++)
        {
            if (isdigit((unsigned char)*q) == 0) return false;
            parsed_port = parsed_port * 10U + (unsigned)(*q - '0');
            if (parsed_port > 65535U) return false;
        }
        if (parsed_port == 0U) return false;
        port = parsed_port;
    }

    const size_t host_len = strlen(auth_buf);
    if (host_len == 0 || host_len > 55) return false;
    if (auth_buf[0] == '.' || auth_buf[0] == '-' || auth_buf[host_len - 1] == '.' || auth_buf[host_len - 1] == '-')
    {
        return false;
    }

    bool has_alnum = false;
    for (size_t i = 0; i < host_len; i++)
    {
        const unsigned char ch = (unsigned char)auth_buf[i];
        if (isalnum(ch))
        {
            has_alnum = true;
            auth_buf[i] = (char)tolower(ch);
        }
        else if (ch == '.' || ch == '-')
        {
            if (ch == '.' && i > 0 && auth_buf[i - 1] == '.') return false;
        }
        else
        {
            return false;
        }
    }
    if (has_alnum == false) return false;

    const int written = snprintf(out, out_len, "%s:%u", auth_buf, port);
    return written > 0 && (size_t)written < out_len;
}

/** @brief 内部辅助: 获取文件签名 (mtime + size) */
static bool read_cfg_file_sig(const char* path, uint64_t* out_mtime, uint64_t* out_size)
{
    if (path == NULL || path[0] == '\0') return false;

#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &fad) == 0) return false;

    ULARGE_INTEGER ft;
    ft.LowPart = fad.ftLastWriteTime.dwLowDateTime;
    ft.HighPart = fad.ftLastWriteTime.dwHighDateTime;

    ULARGE_INTEGER sz;
    sz.LowPart = fad.nFileSizeLow;
    sz.HighPart = fad.nFileSizeHigh;

    *out_mtime = ft.QuadPart;
    *out_size = sz.QuadPart;
    return true;
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;

    *out_mtime = (uint64_t)st.st_mtime;
    *out_size = (uint64_t)st.st_size;
    return true;
#endif
}

/** @brief 在已加锁条件下从数组解析认证服务器到局部缓冲 (返回条数, 0 表示非法/为空) */
static uint8_t parse_auth_servers_array(const cJSON* arr, char out[][AUTH_SERVER_LEN])
{
    if (arr == NULL || cJSON_IsArray(arr) == false) return 0;

    uint8_t count = 0;
    const int n = cJSON_GetArraySize(arr);
    for (int i = 0; i < n && count < AUTH_SERVER_MAX; i++)
    {
        const cJSON* item = cJSON_GetArrayItem(arr, i);
        if (item == NULL || cJSON_IsString(item) == false || item->valuestring == NULL) continue;

        char norm[AUTH_SERVER_LEN];
        if (auth_server_normalize(item->valuestring, norm, sizeof(norm)) == false) continue;

        bool dup = false;
        for (uint8_t k = 0; k < count; k++)
        {
            if (strcmp(out[k], norm) == 0)
            {
                dup = true;
                break;
            }
        }
        if (dup) continue;

        snprintf(out[count], AUTH_SERVER_LEN, "%s", norm);
        count++;
    }
    return count;
}

void auth_servers_defaults(void)
{
    AUTH_SRV_LOCK();
    if (g_prog_status != NULL)
    {
        cfg_t* global = &g_prog_status[0].cfg;
        memset(global->auth_servers, 0, sizeof(global->auth_servers));
        global->auth_server_count = 0;
        for (size_t i = 0; i < DEFAULT_AUTH_SERVER_COUNT && i < AUTH_SERVER_MAX; i++)
        {
            snprintf(global->auth_servers[global->auth_server_count], AUTH_SERVER_LEN, "%s",
                     s_default_auth_servers[i]);
            global->auth_server_count++;
        }
    }
    s_dead_server_count = 0;
    AUTH_SRV_UNLOCK();
}

bool auth_servers_parse(const cJSON* arr)
{
    if (arr == NULL || g_prog_status == NULL) return false;

    if (cJSON_IsArray(arr) == false)
    {
        LOG_WARN("auth_servers 不是数组, 使用内置默认认证服务器");
        return false;
    }

    char parsed[AUTH_SERVER_MAX][AUTH_SERVER_LEN];
    memset(parsed, 0, sizeof(parsed));
    const uint8_t parsed_count = parse_auth_servers_array(arr, parsed);
    if (parsed_count == 0) return false;

    AUTH_SRV_LOCK();
    cfg_t* global = &g_prog_status[0].cfg;
    memset(global->auth_servers, 0, sizeof(global->auth_servers));
    for (uint8_t i = 0; i < parsed_count; i++)
    {
        snprintf(global->auth_servers[i], AUTH_SERVER_LEN, "%.*s", AUTH_SERVER_LEN - 1, parsed[i]);
    }
    global->auth_server_count = parsed_count;
    s_dead_server_count = 0;

    uint64_t mtime_val = 0;
    uint64_t size_val = 0;
    if (read_cfg_file_sig(get_config_path(), &mtime_val, &size_val))
    {
        s_cfg_sig.valid = true;
        s_cfg_sig.mtime_val = mtime_val;
        s_cfg_sig.size_val = size_val;
        s_cfg_sig.last_check_ms = get_steady_tm_ms();
    }
    AUTH_SRV_UNLOCK();

    for (uint8_t i = 0; i < parsed_count; i++)
    {
        LOG_DEBUG("认证服务器: %s", parsed[i]);
    }
    return true;
}

cJSON* auth_servers_to_json(void)
{
    cJSON* arr = cJSON_CreateArray();
    if (arr == NULL) return NULL;

    AUTH_SRV_LOCK();
    if (g_prog_status != NULL && g_prog_status[0].cfg.auth_server_count > 0)
    {
        const cfg_t* global = &g_prog_status[0].cfg;
        for (uint8_t i = 0; i < global->auth_server_count; i++)
        {
            cJSON_AddItemToArray(arr, cJSON_CreateString(global->auth_servers[i]));
        }
    }
    else
    {
        for (size_t i = 0; i < DEFAULT_AUTH_SERVER_COUNT; i++)
        {
            cJSON_AddItemToArray(arr, cJSON_CreateString(s_default_auth_servers[i]));
        }
    }
    AUTH_SRV_UNLOCK();
    return arr;
}

bool auth_servers_reload_if_changed(void)
{
    const char* path = get_config_path();
    if (path == NULL || path[0] == '\0' || g_prog_status == NULL) return false;

    const uint64_t now_ms = get_steady_tm_ms();

    AUTH_SRV_LOCK();
    if (s_cfg_sig.valid && now_ms - s_cfg_sig.last_check_ms < AUTH_RELOAD_CHECK_INTERVAL_MS)
    {
        AUTH_SRV_UNLOCK();
        return false;
    }
    s_cfg_sig.last_check_ms = now_ms;

    uint64_t mtime_val = 0;
    uint64_t size_val = 0;
    if (read_cfg_file_sig(path, &mtime_val, &size_val) == false)
    {
        AUTH_SRV_UNLOCK();
        return false;
    }

    if (s_cfg_sig.valid && s_cfg_sig.mtime_val == mtime_val && s_cfg_sig.size_val == size_val)
    {
        AUTH_SRV_UNLOCK();
        return false;
    }
    AUTH_SRV_UNLOCK();

    char* text = read_cfg_file();
    if (text == NULL) return false;

    cJSON* json = cJSON_Parse(text);
    free(text);
    if (json == NULL)
    {
        LOG_DEBUG("热读取 %s JSON 解析失败 (可能正在写入), 保留当前认证服务器列表", path);
        return false;
    }

    char parsed[AUTH_SERVER_MAX][AUTH_SERVER_LEN];
    memset(parsed, 0, sizeof(parsed));
    const uint8_t parsed_count = parse_auth_servers_array(cJSON_GetObjectItem(json, "auth_servers"), parsed);
    cJSON_Delete(json);

    AUTH_SRV_LOCK();
    /* 重新确认签名已记录, 但只在列表真的变化时打日志与刷新冷却 */
    s_cfg_sig.valid = true;
    s_cfg_sig.mtime_val = mtime_val;
    s_cfg_sig.size_val = size_val;

    bool changed = false;
    if (parsed_count > 0)
    {
        cfg_t* global = &g_prog_status[0].cfg;
        if (global->auth_server_count != parsed_count)
        {
            changed = true;
        }
        else
        {
            for (uint8_t i = 0; i < parsed_count; i++)
            {
                if (strcmp(global->auth_servers[i], parsed[i]) != 0)
                {
                    changed = true;
                    break;
                }
            }
        }

        if (changed)
        {
            memset(global->auth_servers, 0, sizeof(global->auth_servers));
            for (uint8_t i = 0; i < parsed_count; i++)
            {
                snprintf(global->auth_servers[i], AUTH_SERVER_LEN, "%.*s", AUTH_SERVER_LEN - 1, parsed[i]);
            }
            global->auth_server_count = parsed_count;
            s_dead_server_count = 0;
        }
    }
    AUTH_SRV_UNLOCK();

    if (changed)
    {
        LOG_INFO("检测到配置文件变更, 已热更新认证服务器列表 (共 %" PRIu8 " 台)", parsed_count);
    }
    return changed;
}

uint8_t auth_servers_copy_probe_urls(char urls[][AUTH_SERVER_LEN + 16], const uint8_t max_count)
{
    if (urls == NULL || max_count == 0) return 0;

    char list[AUTH_SERVER_MAX][AUTH_SERVER_LEN];
    uint8_t count = 0;

    AUTH_SRV_LOCK();
    if (g_prog_status != NULL && g_prog_status[0].cfg.auth_server_count > 0)
    {
        const cfg_t* global = &g_prog_status[0].cfg;
        for (uint8_t i = 0; i < global->auth_server_count && count < AUTH_SERVER_MAX; i++)
        {
            snprintf(list[count], AUTH_SERVER_LEN, "%s", global->auth_servers[i]);
            count++;
        }
    }
    else
    {
        for (size_t i = 0; i < DEFAULT_AUTH_SERVER_COUNT && count < AUTH_SERVER_MAX; i++)
        {
            snprintf(list[count], AUTH_SERVER_LEN, "%s", s_default_auth_servers[i]);
            count++;
        }
    }
    AUTH_SRV_UNLOCK();

    uint8_t out_count = 0;
    for (uint8_t i = 0; i < count && out_count < max_count; i++)
    {
        char host[AUTH_SERVER_LEN];
        hostport_extract_host(list[i], host, sizeof(host));
        /* CheckNetwork 严格禁止发起系统 DNS 查询, 仅导出 IPv4 字面量 */
        if (is_ipv4_literal(host) == false) continue;

        snprintf(urls[out_count], AUTH_SERVER_LEN + 16, "http://%.*s", AUTH_SERVER_LEN - 1, list[i]);
        out_count++;
    }
    return out_count;
}

void auth_server_add_dynamic(const char* raw)
{
    char norm[AUTH_SERVER_LEN];
    if (auth_server_normalize(raw, norm, sizeof(norm)) == false) return;

    AUTH_SRV_LOCK();
    for (uint8_t i = 0; i < s_dynamic_server_count; i++)
    {
        if (strcmp(s_dynamic_servers[i], norm) == 0)
        {
            AUTH_SRV_UNLOCK();
            return;
        }
    }
    if (s_dynamic_server_count < AUTH_CANDIDATE_MAX)
    {
        snprintf(s_dynamic_servers[s_dynamic_server_count], AUTH_SERVER_LEN, "%s", norm);
        s_dynamic_server_count++;
        LOG_DEBUG("发现动态集群认证节点: %s", norm);
    }
    AUTH_SRV_UNLOCK();
}

/** @brief 从 XML 元素中提取文本并顺手剥掉 CDATA 包裹 */
static char* xml_tag_text(const char* xml, const char* tag)
{
    char* raw = xml_parser(xml, tag);
    if (raw == NULL) return NULL;

    if (strstr(raw, "<![CDATA[") != NULL)
    {
        char* clean = clean_cdata(raw);
        free(raw);
        return clean;
    }
    return raw;
}

void auth_server_extract_econfig(const char* portal_config, const char* last_location)
{
    if (last_location != NULL && last_location[0] != '\0')
    {
        char norm[AUTH_SERVER_LEN];
        if (auth_server_normalize(last_location, norm, sizeof(norm)))
        {
            const char* colon = strchr(norm, ':');
            if (colon != NULL && strcmp(colon + 1, "7001") == 0)
            {
                auth_server_add_dynamic(norm);
            }
        }
    }

    if (portal_config == NULL || portal_config[0] == '\0') return;

    char* auth_url = xml_tag_text(portal_config, "auth-url");
    if (auth_url != NULL)
    {
        auth_server_add_dynamic(auth_url);
        free(auth_url);
    }

    char* ticket_url = xml_tag_text(portal_config, "ticket-url");
    if (ticket_url != NULL)
    {
        auth_server_add_dynamic(ticket_url);
        free(ticket_url);
    }

    /* 解析 <server-clusters servers="..."> 或 <server-clusters>...</server-clusters> */
    const char* cluster_pos = strstr(portal_config, "<server-clusters");
    if (cluster_pos == NULL) return;

    const char* cluster_end = strstr(cluster_pos, "</server-clusters>");
    if (cluster_end == NULL)
    {
        cluster_end = strchr(cluster_pos, '>');
        if (cluster_end == NULL) return;
        cluster_end++;
    }

    const size_t block_len = (size_t)(cluster_end - cluster_pos);
    char* block = malloc(block_len + 1);
    if (block == NULL) return;
    memcpy(block, cluster_pos, block_len);
    block[block_len] = '\0';

    /* 如果写了 servers="..." 属性, 优先从属性值拆节点; 也兼容内嵌列表/子标签 */
    const char* p = block;
    while (*p != '\0')
    {
        while (*p != '\0' && (strchr(" \t\r\n,;\"'<>=", *p) != NULL)) p++;
        if (*p == '\0') break;

        const char* start = p;
        while (*p != '\0' && (strchr(" \t\r\n,;\"'<>", *p) == NULL)) p++;
        const size_t tok_len = (size_t)(p - start);
        if (tok_len > 0 && tok_len < 128)
        {
            char tok[128];
            memcpy(tok, start, tok_len);
            tok[tok_len] = '\0';

            /* 属性 key=value 形如 servers=1.2.3.4:7001 时剥掉等号前缀 */
            const char* eq = strchr(tok, '=');
            const char* cand = (eq != NULL) ? (eq + 1) : tok;
            if (strcmp(cand, "server-clusters") != 0 && strcmp(cand, "/server-clusters") != 0)
            {
                auth_server_add_dynamic(cand);
            }
        }
    }

    free(block);
}

void auth_server_mark_failed(const char* server_url_or_hostport)
{
    char norm[AUTH_SERVER_LEN];
    if (auth_server_normalize(server_url_or_hostport, norm, sizeof(norm)) == false) return;

    const uint64_t now_ms = get_steady_tm_ms();

    AUTH_SRV_LOCK();
    for (uint8_t i = 0; i < s_dead_server_count; i++)
    {
        if (strcmp(s_dead_servers[i].host_port, norm) == 0)
        {
            s_dead_servers[i].failed_at_ms = now_ms;
            AUTH_SRV_UNLOCK();
            return;
        }
    }
    if (s_dead_server_count < AUTH_CANDIDATE_MAX)
    {
        snprintf(s_dead_servers[s_dead_server_count].host_port, AUTH_SERVER_LEN, "%s", norm);
        s_dead_servers[s_dead_server_count].failed_at_ms = now_ms;
        s_dead_server_count++;
    }
    AUTH_SRV_UNLOCK();
}

void auth_server_mark_ok(const char* server_url_or_hostport)
{
    char norm[AUTH_SERVER_LEN];
    if (auth_server_normalize(server_url_or_hostport, norm, sizeof(norm)) == false) return;

    AUTH_SRV_LOCK();
    for (uint8_t i = 0; i < s_dead_server_count; i++)
    {
        if (strcmp(s_dead_servers[i].host_port, norm) == 0)
        {
            if (i + 1 < s_dead_server_count)
            {
                memmove(&s_dead_servers[i], &s_dead_servers[i + 1],
                        sizeof(s_dead_servers[0]) * (size_t)(s_dead_server_count - i - 1));
            }
            s_dead_server_count--;
            break;
        }
    }
    AUTH_SRV_UNLOCK();
}

void auth_server_clear_failures(void)
{
    AUTH_SRV_LOCK();
    s_dead_server_count = 0;
    AUTH_SRV_UNLOCK();
}

/** @brief 内部辅助: 在锁下判断某个 host:port 是否仍在失败冷却期内 */
static bool is_server_in_cooldown_locked(const char* host_port, const uint64_t now_ms)
{
    for (uint8_t i = 0; i < s_dead_server_count; i++)
    {
        if (strcmp(s_dead_servers[i].host_port, host_port) == 0)
        {
            return (now_ms - s_dead_servers[i].failed_at_ms) < AUTH_SERVER_COOLDOWN_MS;
        }
    }
    return false;
}

typedef struct
{
    char host_port[AUTH_SERVER_LEN];
    double connect_time;
    bool in_cooldown;
    bool reachable;
    size_t orig_idx;
} candidate_probe_t;

/** @brief 排序规则: 健康且可连通优先(按连接耗时升序), 其次健康未知/超时, 最后冷却节点 */
static int candidate_rank(const candidate_probe_t* c)
{
    if (!c->in_cooldown && c->reachable) return 0;
    if (!c->in_cooldown) return 1;
    if (c->reachable) return 2;
    return 3;
}

static int compare_candidates(const void* a, const void* b)
{
    const candidate_probe_t* ca = (const candidate_probe_t*)a;
    const candidate_probe_t* cb = (const candidate_probe_t*)b;

    const int ra = candidate_rank(ca);
    const int rb = candidate_rank(cb);
    if (ra != rb) return (ra < rb) ? -1 : 1;

    if (ca->reachable && cb->reachable && ca->connect_time != cb->connect_time)
    {
        return (ca->connect_time < cb->connect_time) ? -1 : 1;
    }
    return (ca->orig_idx < cb->orig_idx) ? -1 : 1;
}

size_t select_auth_servers(char out[][AUTH_SERVER_LEN + 16], const size_t max_count)
{
    if (out == NULL || max_count == 0) return 0;

    candidate_probe_t cands[AUTH_CANDIDATE_MAX];
    memset(cands, 0, sizeof(cands));
    size_t cand_count = 0;

    const uint64_t now_ms = get_steady_tm_ms();

    AUTH_SRV_LOCK();
    /* 1. 先加配置列表里的服务器 */
    if (g_prog_status != NULL && g_prog_status[0].cfg.auth_server_count > 0)
    {
        const cfg_t* global = &g_prog_status[0].cfg;
        for (uint8_t i = 0; i < global->auth_server_count && cand_count < AUTH_CANDIDATE_MAX; i++)
        {
            snprintf(cands[cand_count].host_port, AUTH_SERVER_LEN, "%s", global->auth_servers[i]);
            cands[cand_count].connect_time = -1.0;
            cands[cand_count].in_cooldown = is_server_in_cooldown_locked(cands[cand_count].host_port, now_ms);
            cands[cand_count].orig_idx = cand_count;
            cand_count++;
        }
    }
    else
    {
        for (size_t i = 0; i < DEFAULT_AUTH_SERVER_COUNT && cand_count < AUTH_CANDIDATE_MAX; i++)
        {
            snprintf(cands[cand_count].host_port, AUTH_SERVER_LEN, "%s", s_default_auth_servers[i]);
            cands[cand_count].connect_time = -1.0;
            cands[cand_count].in_cooldown = is_server_in_cooldown_locked(cands[cand_count].host_port, now_ms);
            cands[cand_count].orig_idx = cand_count;
            cand_count++;
        }
    }

    /* 2. 再合并运行期从 EConfig / keep-url 发现的集群节点 (去重) */
    for (uint8_t i = 0; i < s_dynamic_server_count && cand_count < AUTH_CANDIDATE_MAX; i++)
    {
        bool dup = false;
        for (size_t k = 0; k < cand_count; k++)
        {
            if (strcmp(cands[k].host_port, s_dynamic_servers[i]) == 0)
            {
                dup = true;
                break;
            }
        }
        if (dup) continue;

        snprintf(cands[cand_count].host_port, AUTH_SERVER_LEN, "%.*s",
                 AUTH_SERVER_LEN - 1, s_dynamic_servers[i]);
        cands[cand_count].connect_time = -1.0;
        cands[cand_count].in_cooldown = is_server_in_cooldown_locked(cands[cand_count].host_port, now_ms);
        cands[cand_count].orig_idx = cand_count;
        cand_count++;
    }

    /* 若全部候选均处于冷却期, 立即清空冷却以防死锁无节点可试 */
    bool any_healthy = false;
    for (size_t i = 0; i < cand_count; i++)
    {
        if (!cands[i].in_cooldown)
        {
            any_healthy = true;
            break;
        }
    }
    if (any_healthy == false && cand_count > 0)
    {
        s_dead_server_count = 0;
        for (size_t i = 0; i < cand_count; i++)
        {
            cands[i].in_cooldown = false;
        }
        LOG_DEBUG("所有认证服务器均曾失败, 已重置冷却记录重新测速");
    }
    AUTH_SRV_UNLOCK();

    if (cand_count == 0) return 0;

    /* 并发 TCP 测速 (只建连接不发数据, 绑定代理旁路和 OpenWrt SO_MARK) */
    CURLM* multi = curl_multi_init();
    if (multi != NULL)
    {
        CURL* handles[AUTH_CANDIDATE_MAX] = {0};
        size_t handle_cand_idx[AUTH_CANDIDATE_MAX] = {0};
        size_t handle_cnt = 0;

        for (size_t i = 0; i < cand_count; i++)
        {
            char url[AUTH_SERVER_LEN + 16];
            snprintf(url, sizeof(url), "http://%s", cands[i].host_port);

            CURL* handle = curl_easy_init();
            if (handle == NULL) continue;

            curl_easy_setopt(handle, CURLOPT_URL, url);
            curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, AUTH_SERVER_TIMEOUT_MS);
            curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, AUTH_SERVER_TIMEOUT_MS);
            curl_easy_setopt(handle, CURLOPT_CONNECT_ONLY, 1L);
            curl_easy_setopt(handle, CURLOPT_PROXY, "");
            curl_easy_setopt(handle, CURLOPT_NOPROXY, "*");
            curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        #ifdef __OPENWRT__
            curl_easy_setopt(handle, CURLOPT_OPENSOCKETFUNCTION, open_socket_callback);
        #endif

            if (curl_multi_add_handle(multi, handle) != CURLM_OK)
            {
                curl_easy_cleanup(handle);
                continue;
            }

            handles[handle_cnt] = handle;
            handle_cand_idx[handle_cnt] = i;
            handle_cnt++;
        }

        if (handle_cnt > 0)
        {
            int running = 0;
            curl_multi_perform(multi, &running);

            while (running > 0)
            {
                int wait_fds = 0;
                curl_multi_wait(multi, NULL, 0, 200, &wait_fds);
                curl_multi_perform(multi, &running);

                CURLMsg* msg = NULL;
                int left = 0;
                while ((msg = curl_multi_info_read(multi, &left)) != NULL)
                {
                    if (msg->msg != CURLMSG_DONE || msg->data.result != CURLE_OK) continue;

                    for (size_t h = 0; h < handle_cnt; h++)
                    {
                        if (handles[h] != msg->easy_handle) continue;

                        double seconds = 0.0;
                        if (curl_easy_getinfo(msg->easy_handle, CURLINFO_CONNECT_TIME, &seconds) == CURLE_OK)
                        {
                            const size_t ci = handle_cand_idx[h];
                            cands[ci].connect_time = seconds;
                            cands[ci].reachable = true;
                        }
                    }
                }
            }
        }

        for (size_t h = 0; h < handle_cnt; h++)
        {
            curl_multi_remove_handle(multi, handles[h]);
            curl_easy_cleanup(handles[h]);
        }
        curl_multi_cleanup(multi);
    }

    qsort(cands, cand_count, sizeof(cands[0]), compare_candidates);

    const size_t out_count = (cand_count < max_count) ? cand_count : max_count;
    for (size_t i = 0; i < out_count; i++)
    {
        snprintf(out[i], AUTH_SERVER_LEN + 16, "http://%.*s", AUTH_SERVER_LEN - 1, cands[i].host_port);
    }

    static _Thread_local char s_last_server[AUTH_SERVER_LEN + 16] = "";
    if (strcmp(s_last_server, out[0]) != 0)
    {
        snprintf(s_last_server, sizeof(s_last_server), "%s", out[0]);
        if (cands[0].reachable)
        {
            LOG_INFO("选择认证服务器: %s (连接耗时 %.0f ms, 候选共 %zu 台)",
                     cands[0].host_port, cands[0].connect_time * 1000.0, cand_count);
        }
        else
        {
            LOG_WARN("候选认证服务器均未能测通 (%zu 台), 先按 %s 试", cand_count, cands[0].host_port);
        }
    }
    else
    {
        LOG_DEBUG("认证服务器: %s (候选共 %zu 台)", out[0], cand_count);
    }

    return out_count;
}

bool select_auth_server(char* out, const size_t out_len)
{
    if (out == NULL || out_len == 0) return false;
    out[0] = '\0';

    char list[1][AUTH_SERVER_LEN + 16];
    const size_t cnt = select_auth_servers(list, 1);
    if (cnt == 0) return false;

    snprintf(out, out_len, "%s", list[0]);
    return true;
}
