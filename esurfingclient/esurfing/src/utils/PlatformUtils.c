#include "utils/PlatformUtils.h"
#include "utils/Logger.h"
#include "utils/cJSON.h"
#include "States.h"

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32

#include <sysinfoapi.h>
#include <iphlpapi.h>

#endif

static const char s_default_cfg[] = "{\n"
                                    "   \"log_lv\": 4,\n"
                                    "   \"accounts\": [\n"
                                    "       {\n"
                                    "           \"username\": \"\",\n"
                                    "           \"password\": \"\",\n"
                                    "           \"channel\": \"phone\"\n"
                                    // "           \"auto_start\": false\n"
                                    "       }\n"
                                    "   ]\n"
                                    "}\n";

char* xml_parser(const char* xml_data, const char* tag)
{
    if (xml_data == NULL || tag == NULL) return NULL;

    char start_tag[256];
    snprintf(start_tag, sizeof(start_tag), "<%s>", tag);

    char end_tag[256];
    snprintf(end_tag, sizeof(end_tag), "</%s>", tag);

    const char* start_pos = strstr(xml_data, start_tag);
    if (!start_pos) return NULL;
    start_pos += strlen(start_tag);

    const char* end_pos = strstr(start_pos, end_tag);
    if (!end_pos) return NULL;

    const size_t content_length = end_pos - start_pos;
    if (content_length <= 0) return NULL;

    char* content = malloc(content_length + 1);
    if (!content) return NULL;

    strncpy(content, start_pos, content_length);
    content[content_length] = '\0';
    return content;
}

bytes_t str_2_bytes(const char* str)
{
    bytes_t ba = {0};
    if (!str) return ba;
    ba.length = strlen(str);
    ba.data = (uint8_t*)malloc(ba.length);
    if (ba.data) memcpy(ba.data, str, ba.length);
    return ba;
}

uint64_t str_2_uint64(const char* str)
{
    if (!str) return 0;
    while (isspace(*str)) str++;
    if (*str == '\0') return 0;
    char* end_ptr;
    errno = 0;
    const uint64_t value = strtoll(str, &end_ptr, 10);
    if (errno == ERANGE) return 0;
    if (end_ptr == str) return 0;
    while (isspace(*end_ptr)) end_ptr++;
    if (*end_ptr != '\0') return 0;
    return value;
}

char* uint64_2_str(const uint64_t num)
{
    char* result = malloc(22);
    if (!result) return NULL;
    snprintf(result, 22, "%" PRIu64, num);
    return result;
}

uint64_t get_cur_tm_ms()
{
#ifdef _WIN32
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart / 10000LL - 11644473600000LL;
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) return 0;
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
#endif
}

void get_rand_bytes(uint8_t* buf, const size_t len)
{
#ifdef _WIN32
    HCRYPTPROV h_crypt_prov;
    if (!CryptAcquireContext(&h_crypt_prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return;
    CryptGenRandom(h_crypt_prov, len, buf);
    CryptReleaseContext(h_crypt_prov, 0);
#else
    const int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) return;
    read(fd, buf, len);
    close(fd);
#endif
}

void sleep_ms(const uint64_t ms)
{
    if (ms <= 0) return;
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void get_fmt_time(char* buf, const TimeFormat fmt)
{
    time_t raw_tm;
    if (time(&raw_tm) == (time_t) - 1)
    {
        fprintf(stderr, "ERROR: 获取系统时间失败\n");
        return;
    }
    struct tm local_tm;
#ifdef _WIN32
    if (localtime_s(&local_tm, &raw_tm) != 0)
    {
        fprintf(stderr, "ERROR: 时间转换失败\n");
        return;
    }
#else
    if (localtime_r(&raw_tm, &local_tm) == NULL)
    {
        fprintf(stderr, "ERROR: 时间转换失败\n");
        return;
    }
#endif
    switch (fmt)
    {
    case CONSOLE_FORMAT:
        if (strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &local_tm) == 0)
        {
            fprintf(stderr, "ERROR: 格式化时间失败\n");
            return;
        }
        return;
    case FILE_FORMAT:
        if (strftime(buf, 32, "%Y%m%d-%H%M%S", &local_tm) == 0)
        {
            fprintf(stderr, "ERROR: 格式化时间失败\n");
        }
    }
}

const char* safe_str(const char* str)
{
    return str ? str : "";
}

char* create_xml_payload(const XmlChoose choose)
{
    char cur_tm[32];
    get_fmt_time(cur_tm, CONSOLE_FORMAT);
    static char xml[XML_BUFFER_SIZE] = "";
    LOG_DEBUG("XML 选择代码: %d", choose);
    uint16_t xml_len = 0;
    switch (choose)
    {
    case GET_TICKET:
        xml_len = snprintf(xml, XML_BUFFER_SIZE,
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<request>\n"
            "    <user-agent>%s</user-agent>\n"
            "    <client-id>%s</client-id>\n"
            "    <local-time>%s</local-time>\n"
            "    <host-name>%s</host-name>\n"
            "    <ipv4>%s</ipv4>\n"
            "    <ipv6></ipv6>\n"
            "    <mac>%s</mac>\n"
            "    <ostag>%s</ostag>\n"
            "    <gwip>%s</gwip>\n"
            "</request>\n",
            safe_str(g_prog_status[thread_idx].login_cfg.user_agent),
            safe_str(g_prog_status[thread_idx].auth_cfg.client_id),
            safe_str(cur_tm),
            safe_str(g_prog_status[thread_idx].auth_cfg.host_name),
            safe_str(g_prog_status[thread_idx].auth_cfg.client_ip),
            safe_str(g_prog_status[thread_idx].auth_cfg.mac_addr),
            safe_str(g_prog_status[thread_idx].auth_cfg.host_name),
            safe_str(g_prog_status[thread_idx].auth_cfg.ac_ip)
        );
        break;
    case LOGIN:
        xml_len = snprintf(xml, XML_BUFFER_SIZE,
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<request>\n"
            "    <user-agent>%s</user-agent>\n"
            "    <client-id>%s</client-id>\n"
            "    <ticket>%s</ticket>\n"
            "    <local-time>%s</local-time>\n"
            "    <userid>%s</userid>\n"
            "    <passwd>%s</passwd>\n"
            "</request>\n",
            safe_str(g_prog_status[thread_idx].login_cfg.user_agent),
            safe_str(g_prog_status[thread_idx].auth_cfg.client_id),
            safe_str(g_prog_status[thread_idx].auth_cfg.ticket),
            safe_str(cur_tm),
            safe_str(g_prog_status[thread_idx].login_cfg.usr),
            safe_str(g_prog_status[thread_idx].login_cfg.pwd)
        );
        break;
    case HEART_BEAT:
    case TERM:
        xml_len = snprintf(xml, XML_BUFFER_SIZE,
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<request>\n"
            "    <user-agent>%s</user-agent>\n"
            "    <client-id>%s</client-id>\n"
            "    <local-time>%s</local-time>\n"
            "    <host-name>%s</host-name>\n"
            "    <ipv4>%s</ipv4>\n"
            "    <ticket>%s</ticket>\n"
            "    <ipv6></ipv6>\n"
            "    <mac>%s</mac>\n"
            "    <ostag>%s</ostag>\n"
            "</request>\n",
            safe_str(g_prog_status[thread_idx].login_cfg.user_agent),
            safe_str(g_prog_status[thread_idx].auth_cfg.client_id),
            safe_str(cur_tm),
            safe_str(g_prog_status[thread_idx].auth_cfg.host_name),
            safe_str(g_prog_status[thread_idx].auth_cfg.client_ip),
            safe_str(g_prog_status[thread_idx].auth_cfg.ticket),
            safe_str(g_prog_status[thread_idx].auth_cfg.mac_addr),
            safe_str(g_prog_status[thread_idx].auth_cfg.host_name)
        );
        break;
    default:
        LOG_ERROR("XML 选择代码错误");
        return NULL;
    }
    if (xml_len <= 0)
    {
        LOG_ERROR("XML 创建失败");
        return NULL;
    }
    if (xml_len >= XML_BUFFER_SIZE)
    {
        LOG_ERROR("XML 内容过长 (需要 %d 字节，但缓冲区只有 %d 字节)", xml_len + 1, XML_BUFFER_SIZE);
        return NULL;
    }
    LOG_DEBUG("创建 XML 完成");
    LOG_VERBOSE("XML 内容为:\n%s", xml);
    return xml;
}

char* extract_between_tags(const char* text, const char* start_tag, const char* end_tag)
{
    if (!text)
    {
        LOG_ERROR("传入文本为空");
        return NULL;
    }
    char* start = strstr(text, start_tag);
    if (!start)
    {
        LOG_ERROR("未找到开头标签: %s", start_tag);
        return NULL;
    }
    start += strlen(start_tag);
    char* end = strstr(start, end_tag);
    if (!end)
    {
        LOG_WARN("未找到结尾标签: %s, 返回", end_tag);
        return NULL;
    }
    const size_t len = end - start;
    if (len == 0) LOG_WARN("提取到空内容 (标签: %s...%s)", start_tag, end_tag);
    char* result = malloc(len + 1);
    if (!result)
    {
        LOG_ERROR("分配内存失败");
        return NULL;
    }
    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

char* clean_CDATA(const char* text)
{
    return extract_between_tags(text, "<![CDATA[", "]]>");
}

// bool save_cfg()
// {
//     cJSON* cfg_json = cJSON_CreateObject();
//
//     cJSON_AddNumberToObject(cfg_json, "log_lv", get_logger_level());
//
//     cJSON_AddBoolToObject(cfg_json, "use_cus_if", g_use_cus_ip);
//
//     cJSON* accounts = cJSON_CreateArray();
//     cJSON_AddItemToObject(cfg_json, "accounts", accounts);
//
//     for (uint8_t i = 0; i < g_prog_cnt; i++)
//     {
//         cJSON* account = cJSON_CreateObject();
//
//         cJSON_AddStringToObject(account, "username", g_prog_status[i].login_cfg.usr);
//         cJSON_AddStringToObject(account, "password", g_prog_status[i].login_cfg.pwd);
//         cJSON_AddStringToObject(account, "channel", g_prog_status[i].login_cfg.chn);
//         cJSON_AddStringToObject(account, "bind_if", g_prog_status[i].login_cfg.if);
//         // cJSON_AddBoolToObject(account, "auto_start", g_prog_status[i].login_cfg.auto_start);
//
//         cJSON_AddItemToArray(accounts, account);
//     }
//
//     char* json = cJSON_Print(cfg_json);
//
//     FILE* cfg_file = fopen(DIALER_CONFIG_FILE, "w");
//     if (!cfg_file)
//     {
//         LOG_ERROR("无法生成文件: %s", DIALER_CONFIG_FILE);
//         return false;
//     }
//     fprintf(cfg_file, "%s", json);
//     fclose(cfg_file);
//
//     free(json);
//     cJSON_Delete(cfg_json);
//     return true;
// }

#ifdef __OPENWRT__
static bool load_cfg_openwrt(cJSON* cfg_json)
{
    LOG_INFO("当前环境为 OpenWRT 或其衍生系统, 将会使用独有配置加载方式");

    cJSON* accounts = cJSON_GetObjectItem(cfg_json, "accounts");
    if (!accounts || !cJSON_IsArray(accounts) || cJSON_GetArraySize(accounts) == 0)
    {
        LOG_FATAL("没有找到账号数据, 请添加");
        cJSON_Delete(cfg_json);
        return false;
    }

    const uint8_t cnt = cJSON_GetArraySize(accounts);

    prog_status_t* new_prog_status = realloc(g_prog_status, sizeof(prog_status_t) * cnt);
    if (new_prog_status)
    {
        g_prog_status = new_prog_status;
        memset(g_prog_status, 0, sizeof(prog_status_t) * cnt);
    }

    int8_t valid_cnt = 0;

    for (uint8_t i = 0, valid_i = 0; i < cnt; i++)
    {
        const cJSON* account = cJSON_GetArrayItem(accounts, i);

        const cJSON* usr = cJSON_GetObjectItem(account, "username");
        const cJSON* pwd = cJSON_GetObjectItem(account, "password");
        const cJSON* chn = cJSON_GetObjectItem(account, "channel");
        // const cJSON* auto_start = cJSON_GetObjectItem(account, "auto_start");

        if (usr->valuestring[0] != '\0' && pwd->valuestring[0] != '\0' && chn->valuestring[0] != '\0')
        {
            snprintf(g_prog_status[valid_i].login_cfg.usr, USR_LEN, "%s", safe_str(usr->valuestring));
            snprintf(g_prog_status[valid_i].login_cfg.pwd, PWD_LEN, "%s", safe_str(pwd->valuestring));
            snprintf(g_prog_status[valid_i].login_cfg.chn, CHN_LEN, "%s", safe_str(chn->valuestring));
            // g_prog_status[valid_i].login_cfg.auto_start = auto_start->valueint;
            if (strcmp(chn->valuestring, "pc") == 0)
            {
                snprintf(g_prog_status[valid_i].login_cfg.user_agent, USER_AGENT_LEN, "CCTP/Linux64/1003");
                LOG_DEBUG("使用 UA: %s", g_prog_status[valid_i].login_cfg.user_agent);
                LOG_DEBUG("当前使用下标: %" PRIu8, valid_i);
            }
            else
            {
                snprintf(g_prog_status[valid_i].login_cfg.user_agent, USER_AGENT_LEN, "CCTP/android64_vpn/2093");
                LOG_DEBUG("使用 UA: %s", g_prog_status[valid_i].login_cfg.user_agent);
                LOG_DEBUG("当前使用下标: %" PRIu8, valid_i);
            }
            g_prog_status[valid_i].login_cfg.idx = i + 1;
            g_prog_status[valid_i].mark = 0x100 + valid_i * 0x100;
            LOG_INFO("配置 %" PRIu8 " 可用, 将会尝试使用", i + 1);
            valid_cnt++;
            valid_i++;
        }
        else
        {
            LOG_WARN("配置 %" PRIu8 " 一个或多个值为空, 跳过当前配置", i + 1);
        }
    }

    cJSON_Delete(cfg_json);

    if (valid_cnt == 0)
    {
        LOG_FATAL("无可用配置");
        return false;
    }

    g_prog_cnt = valid_cnt;

    LOG_INFO("可用配置数: %" PRId8, g_prog_cnt);
    return true;
}
#else
static bool load_cfg_other(cJSON* cfg_json)
{
    LOG_INFO("当前环境为普通系统, 将会使用通用配置加载方式, 仅会尝试加载第一个有效配置");

    cJSON* accounts = cJSON_GetObjectItem(cfg_json, "accounts");
    if (!accounts || !cJSON_IsArray(accounts) || cJSON_GetArraySize(accounts) == 0)
    {
        LOG_FATAL("没有找到账号数据, 请添加");
        cJSON_Delete(cfg_json);
        return false;
    }

    const uint8_t cnt = cJSON_GetArraySize(accounts);

    int8_t valid_cnt = 0;

    for (uint8_t i = 0; i < cnt; i++)
    {
        const cJSON* account = cJSON_GetArrayItem(accounts, i);

        const cJSON* usr = cJSON_GetObjectItem(account, "username");
        const cJSON* pwd = cJSON_GetObjectItem(account, "password");
        const cJSON* chn = cJSON_GetObjectItem(account, "channel");
        // const cJSON* auto_start = cJSON_GetObjectItem(account, "auto_start");

        if (!usr || !pwd || !chn)
        {
            LOG_INFO("");
            return false;
        }

        if (usr->valuestring[0] != '\0' && pwd->valuestring[0] != '\0' && chn->valuestring[0] != '\0')
        {
            // if_nametoindex(i_f->valuestring) != 0
            snprintf(g_prog_status[0].login_cfg.usr, USR_LEN, "%s", safe_str(usr->valuestring));
            snprintf(g_prog_status[0].login_cfg.pwd, PWD_LEN, "%s", safe_str(pwd->valuestring));
            snprintf(g_prog_status[0].login_cfg.chn, CHN_LEN, "%s", safe_str(chn->valuestring));
            // g_prog_status[0].login_cfg.auto_start = auto_start->valueint;
            if (strcmp(chn->valuestring, "pc") == 0)
            {
                snprintf(g_prog_status[0].login_cfg.user_agent, USER_AGENT_LEN, "CCTP/Linux64/1003");
                LOG_DEBUG("使用 UA: %s", g_prog_status[0].login_cfg.user_agent);
                LOG_DEBUG("当前使用下标: 0");
            }
            else
            {
                snprintf(g_prog_status[0].login_cfg.user_agent, USER_AGENT_LEN, "CCTP/android64_vpn/2093");
                LOG_DEBUG("使用 UA: %s", g_prog_status[0].login_cfg.user_agent);
                LOG_DEBUG("当前使用下标: 0");
            }
            g_prog_status[0].login_cfg.idx = i + 1;
            LOG_INFO("配置 %" PRIu8 " 可用, 将会尝试使用", i + 1);
            valid_cnt++;
            break;
        }
        LOG_WARN("配置 %" PRIu8 " 一个或多个值为空, 跳过当前配置", i + 1);
    }

    cJSON_Delete(cfg_json);

    if (valid_cnt == 0)
    {
        LOG_FATAL("无可用配置");
        return false;
    }

    g_prog_cnt = valid_cnt;

    LOG_INFO("可用配置数: %" PRId8, g_prog_cnt);
    return true;
}
#endif

bool load_cfg()
{
    FILE* cfg_file = fopen(DIALER_CONFIG_FILE, "r");
    if (!cfg_file || fgetc(cfg_file) == EOF)
    {
        LOG_ERROR("无法打开配置文件或配置文件为空: %s", DIALER_CONFIG_FILE);
        LOG_INFO("创建新的默认配置文件");
        FILE* new_cfg = fopen(DIALER_CONFIG_FILE, "w");
        if (!new_cfg)
        {
            LOG_FATAL("无法生成文件: %s", DIALER_CONFIG_FILE);
            return false;
        }
        fprintf(new_cfg, "%s", s_default_cfg);
        fclose(new_cfg);
        LOG_INFO("创建完成, 请在 %s 填写账号数据", DIALER_CONFIG_FILE);
        return false;
    }

    fseek(cfg_file, 0, SEEK_END);
    const long len = ftell(cfg_file);
    fseek(cfg_file, 0, SEEK_SET);

    char* cfg_data = malloc(len + 1);
    fread(cfg_data, 1, len, cfg_file);
    cfg_data[len] = '\0';
    fclose(cfg_file);

    cJSON* cfg_json = cJSON_Parse(cfg_data);
    free(cfg_data);
    if (!cfg_json)
    {
        LOG_FATAL("JSON 解析失败");
        return false;
    }

    const cJSON* log_lv = cJSON_GetObjectItem(cfg_json, "log_lv");
    if (log_lv && cJSON_IsNumber(log_lv))
    {
        set_logger_level(log_lv->valueint);
    }
    else
    {
        LOG_WARN("日志等级读取失败, 使用默认等级 (INFO)");
    }

#ifdef __OPENWRT__
    if (load_cfg_openwrt(cfg_json) == false) return false;
#else
    if (load_cfg_other(cfg_json) == false) return false;
#endif

    return true;
}
