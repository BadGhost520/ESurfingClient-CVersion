#include "net/NetClient.h"

#include "utils/log/Logger.h"
#include "utils/sim/SimThread.h"
#include "utils/time/TimeUtils.h"

#include <curl/curl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define GENERATE_URL "http://connectivitycheck.platform.hicloud.com/generate_204"
#define GENERATE_BAK_URL "http://connect.rom.miui.com/generate_204"
#define AUTH_URL "http://14.146.227.141:7001"
#define AUTH_BAK_URL "http://121.8.177.212:7001"

/** 一次并发探测的 handle 上限 (每个目标可能同时开 DNS 一路 + 多个固定 IP) */
#define PROBE_MAX 8

/** 每个域名最多配几个兜底 IP (一个域名有多条 A 记录很正常) */
#define KNOWN_IP_MAX 2

/**
 * 单个探测的超时 (毫秒)
 *
 * 探测只关心"连不连得上、状态码是多少", 正常都在几十毫秒内完成。
 * 原来沿用的是配置里的 conn_timeout (常见 7~10 秒) 串行试 2~4 个地址: 一旦某个
 * 地址被网关/防火墙静默丢弃 (SYN 发出去没有回应), 一次检测就要白等十几到几十秒。
 * 这类超时与服务器响应快慢无关, 调大超时只会等更久 —— 所以这里固定用短超时。
 */
#define PROBE_CONNECT_TIMEOUT_MS 3000L
#define PROBE_TOTAL_TIMEOUT_MS 4000L

/** 后台探测的间隔 (仅 Web 进程用) */
#define PROBE_INTERVAL_MS 2000

/** 失败汇总的最小间隔: 探测 2 秒一轮, 网络长时间不通时不能把日志刷满 */
#define PROBE_FAIL_REPORT_MS 30000

/** 丢弃响应体: 只需要状态码, 不把门户页面整个拉下来 */
static size_t probe_sink(const char* ptr, size_t size, size_t nmemb, void* userdata)
{
    (void)ptr;
    (void)userdata;

    return size * nmemb;
}

/** 一个探测目标 */
typedef struct
{
    const char* url;
    const char* label;
} probe_target_t;

/**
 * 域名的兜底解析表
 *
 * 校园网里 DNS 经常根本不可用 (未认证时被拦, curl 报 CURLcode 6 解析超时),
 * 所以探测时除了走 DNS, 再并上一路用固定 IP 的请求 —— 谁先出结果就用谁:
 * - DNS 能解析: 用 DNS 拿到的最新 IP (不会因为固定 IP 过期而误判)
 * - DNS 解析不了: 固定 IP 顶上 (这是原来唯一的做法)
 */
typedef struct
{
    const char* host;
    /** 兜底 IP, 按顺序; 每个 IP 会各开一路并发探测, 空位填 NULL */
    const char* ips[KNOWN_IP_MAX];
} known_host_t;

static const known_host_t s_known_hosts[] = {
    {
        "connectivitycheck.platform.hicloud.com",
        {"183.61.185.93", "125.88.255.53"}
        // {"183.61.185.93", "125.88.255.53", "183.61.228.80", "183.61.228.74", "14.29.44.97", "183.61.225.71", "14.29.44.94", "125.88.255.54", "183.61.225.72", "183.61.185.106"}
    },
    {
        "connect.rom.miui.com",
        {"202.69.4.32", "220.181.104.183"}
    },
};

/** 一个在飞的探测 */
typedef struct
{
    CURL* handle;
    struct curl_slist* resolve;
    const char* label;
    CURLcode code;
} probe_t;

/**
 * @brief 从 URL 里取出 host (http://host[:port]/...)
 * @return 是否取到
 */
static bool url_host(const char* url, char* out, size_t out_len)
{
    if (url == NULL || out == NULL || out_len == 0) return false;

    const char* start = strstr(url, "://");
    start = (start != NULL) ? start + 3 : url;

    const size_t len = strcspn(start, ":/");
    if (len == 0 || len >= out_len) return false;

    memcpy(out, start, len);
    out[len] = '\0';
    return true;
}

/** @brief 找某个 host 的兜底 IP 表, 没有就返回 NULL */
static const known_host_t* known_host_for(const char* host)
{
    if (host == NULL) return NULL;

    for (size_t i = 0; i < sizeof(s_known_hosts) / sizeof(s_known_hosts[0]); i++)
    {
        if (strcmp(s_known_hosts[i].host, host) == 0) return &s_known_hosts[i];
    }
    return NULL;
}

/**
 * @brief 往 multi 里加一个探测
 * @param ip 非空时用固定解析 (CURLOPT_RESOLVE), 为空则走 DNS
 */
static bool probe_add(CURLM* multi, const char* url, const char* host, const char* ip,
                      const char* label, probe_t* probes, size_t* count)
{
    if (*count >= PROBE_MAX) return false;

    CURL* handle = curl_easy_init();
    if (handle == NULL) return false;

    struct curl_slist* resolve = NULL;

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, PROBE_CONNECT_TIMEOUT_MS);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, PROBE_TOTAL_TIMEOUT_MS);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, probe_sink);
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
    /**
     * 只要 IPv4
     *
     * 这些检测域名的 AAAA 记录很常见 (实测 hicloud 的 DNS 只回 IPv6), 而校园网/
     * 路由器上 IPv6 往往根本不通。让 curl 先试 IPv6, 每次都要白等一次连接超时
     * 才回落到 IPv4 —— 表现就是"检测经常超时"。
     */
    curl_easy_setopt(handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    if (ip != NULL && host != NULL)
    {
        char entry[256];
        snprintf(entry, sizeof(entry), "%s:80:%s", host, ip);

        resolve = curl_slist_append(NULL, entry);
        if (resolve == NULL)
        {
            curl_easy_cleanup(handle);
            return false;
        }
        curl_easy_setopt(handle, CURLOPT_RESOLVE, resolve);
    }

    if (curl_multi_add_handle(multi, handle) != CURLM_OK)
    {
        curl_slist_free_all(resolve);
        curl_easy_cleanup(handle);
        return false;
    }

    probes[*count].handle = handle;
    probes[*count].resolve = resolve;
    probes[*count].label = label;
    probes[*count].code = CURLE_OK;
    (*count)++;

    return true;
}

/**
 * @brief 汇总一次"全部失败"的原因 (带限流)
 *
 * 只打结果不说明是哪一路失败的话, 排查时还得猜是 DNS 超时还是连接超时。
 * @param probes 本次所有在飞的探测
 */
static void probe_report_failure(const probe_t* probes, const size_t count)
{
    static uint64_t s_last_report_ms = 0;

    const uint64_t now = get_steady_tm_ms();
    if (now - s_last_report_ms < PROBE_FAIL_REPORT_MS) return;
    s_last_report_ms = now;

    char detail[256];
    size_t used = 0;
    detail[0] = '\0';

    for (size_t i = 0; i < count && used + 24 < sizeof(detail); i++)
    {
        const int n = snprintf(detail + used, sizeof(detail) - used, "%s=%d ",
                               probes[i].label, (int)probes[i].code);
        if (n <= 0) break;
        used += (size_t)n;
    }

    LOG_WARN("联网检测全部失败: %s(6=域名解析失败, 28=连接超时, 7=连不上)", detail);
}

/**
 * @brief 并发探测一组目标, 谁先给出可用结果就采用谁
 *
 * 为什么要并发:
 * - 这些 generate_204 地址在不同线路上的可达性差别很大 (某路由器一天里主地址连接
 *   超时 183 次、备用地址只失败 4 次), 串行试的话一个不可达地址就要吃满一次超时;
 * - 同一目标同时开 DNS 与固定 IP 两路 —— 校园网 DNS 可能根本解析不出来, 而固定 IP
 *   可能已经过期不可达, 两路都备着就不怕其中一种坏掉;
 * - 固定 IP 可以配多个 (一个域名对应多条 A 记录很正常), 每个 IP 各占一路并发 ——
 *   写成一条 `CURLOPT_RESOLVE` 多地址的话 curl 会串行试, 一样要等超时。
 *
 * @return STATUS_OK 拿到 204/301, STATUS_NEED_AUTH 拿到 302, 其余 STATUS_ERROR
 */
static network_status_t probe_group(const probe_target_t* targets, const size_t count)
{
    if (targets == NULL || count == 0) return STATUS_ERROR;

    CURLM* multi = curl_multi_init();
    if (multi == NULL) return STATUS_ERROR;

    probe_t probes[PROBE_MAX];
    char labels[PROBE_MAX][64];
    size_t probe_count = 0;
    network_status_t result = STATUS_ERROR;

    for (size_t i = 0; i < count && probe_count < PROBE_MAX; i++)
    {
        char host[128] = "";
        const bool has_host = url_host(targets[i].url, host, sizeof(host));
        const known_host_t* known = has_host ? known_host_for(host) : NULL;

        snprintf(labels[probe_count], sizeof(labels[0]), "%s/dns", targets[i].label);
        probe_add(multi, targets[i].url, host, NULL, labels[probe_count], probes, &probe_count);

        if (known != NULL)
        {
            for (size_t k = 0; k < KNOWN_IP_MAX && known->ips[k] != NULL && probe_count < PROBE_MAX; k++)
            {
                snprintf(labels[probe_count], sizeof(labels[0]), "%s/ip%zu", targets[i].label, k + 1);
                probe_add(multi, targets[i].url, host, known->ips[k], labels[probe_count], probes, &probe_count);
            }
        }
    }

    if (probe_count > 0)
    {
        const uint64_t deadline = get_steady_tm_ms() + PROBE_TOTAL_TIMEOUT_MS + 1000;
        int running = 0;

        curl_multi_perform(multi, &running);

        while (running > 0 && get_steady_tm_ms() < deadline)
        {
            int wait_fds = 0;
            curl_multi_wait(multi, NULL, 0, 200, &wait_fds);
            curl_multi_perform(multi, &running);

            CURLMsg* msg = NULL;
            int left = 0;
            while ((msg = curl_multi_info_read(multi, &left)) != NULL)
            {
                if (msg->msg != CURLMSG_DONE) continue;

                for (size_t i = 0; i < probe_count; i++)
                {
                    if (probes[i].handle == msg->easy_handle)
                    {
                        probes[i].code = msg->data.result;
                    }
                }

                long http_code = 0;
                curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &http_code);

                if (http_code == HTTP_NO_CONTENT || http_code == HTTP_MOVED_PERMANENTLY)
                {
                    result = STATUS_OK;
                    running = 0;
                    break;
                }
                if (http_code == HTTP_FOUND)
                {
                    result = STATUS_NEED_AUTH;
                    running = 0;
                    break;
                }
            }
        }

        if (result == STATUS_ERROR) probe_report_failure(probes, probe_count);
    }

    for (size_t i = 0; i < probe_count; i++)
    {
        curl_multi_remove_handle(multi, probes[i].handle);
        curl_easy_cleanup(probes[i].handle);
        curl_slist_free_all(probes[i].resolve);
    }
    curl_multi_cleanup(multi);

    return result;
}

network_status_t check_network_status(const bool connect_only)
{
    /**
     * connect_only 只为兼容调用点保留: 探测统一发完整 GET 再看状态码 ——
     * 只建连接不请求的话拿不到 204/302, 也就分不清"能上网"和"需要认证"。
     */
    (void)connect_only;

    static const probe_target_t internet_targets[] = {
        {GENERATE_URL, "hicloud"},
        {GENERATE_BAK_URL, "miui"},
    };
    static const probe_target_t auth_targets[] = {
        {AUTH_URL, "auth1"},
        {AUTH_BAK_URL, "auth2"},
    };

    const network_status_t status =
        probe_group(internet_targets, sizeof(internet_targets) / sizeof(internet_targets[0]));
    if (status != STATUS_ERROR) return status;

    /**
     * 外网都不通: 再并发试认证服务器 —— 能连上 (返回 302 或其它状态码) 说明
     * 人还在校园网里, 只是没过认证。
     */
    return probe_group(auth_targets, sizeof(auth_targets) / sizeof(auth_targets[0]));
}

/**
 * @brief 网络状态变化时提示一次
 *
 * 这些状态是每秒/每几秒轮询一次的, 每次都打 INFO 会把日志刷满 —— 但完全不提示
 * 又不行: 用户至少要知道"现在已联网, 不需要认证"。所以只在状态真的变了时提示
 * 一次, 相同的状态重复检测到就什么都不打。
 *
 * 状态按线程记录: 认证进程里每个账号一个线程, 别让它们互相覆盖。
 *
 * @param status 本次检测到的网络状态
 */
void log_network_state_change(const network_status_t status)
{
    static _Thread_local network_status_t s_last_state = (network_status_t)-1;

    if (status == s_last_state) return;

    s_last_state = status;

    switch (status)
    {
    case STATUS_OK:
        LOG_INFO("已连接至互联网");
        break;
    case STATUS_NEED_AUTH:
        LOG_INFO("需要认证");
        break;
    default:
        // 网络异常的具体原因由调用点的 WARN/ERROR 覆盖, 这里不重复
        break;
    }
}

/* ------------------------------------------------------------------
 * 后台联网探测 (仅 Web 进程用)
 * Web 线程是单线程的事件循环, 在这里现做一次探测 (最坏十几秒) 会把
 * /api/traffic、/api/hotspot/status 一起拖到前端超时, 所以改成后台线程定时探、
 * 接口只读缓存。
 * ------------------------------------------------------------------ */

static volatile bool s_probe_running = false;
static network_status_t s_probe_cached = STATUS_ERROR;
static sim_thread_t* s_probe_thread = NULL;

static int probe_thread(void* arg)
{
    (void)arg;

    while (s_probe_running)
    {
        const uint64_t started = get_steady_tm_ms();
        s_probe_cached = check_network_status(true);
        const uint64_t elapsed = get_steady_tm_ms() - started;

        /**
         * 正常都在几十毫秒内完成。超过 1 秒说明有检测地址不可达 (SYN 被静默丢弃,
         * 只能等满超时) —— 这正是"经常超时"的现场, 留一条 WARN 方便排查。
         */
        if (elapsed >= 1000)
        {
            LOG_WARN("联网状态探测耗时 %" PRIu64 " 毫秒 (结果 %d): 有检测地址不可达, 已按短超时放弃并换下一个",
                     elapsed, (int)s_probe_cached);
        }

        for (int i = 0; i < PROBE_INTERVAL_MS / 100; i++)
        {
            if (s_probe_running == false) return 0;
            sleep_ms(100, false);
        }
    }

    return 0;
}

void net_probe_start(void)
{
    if (s_probe_thread != NULL) return;

    s_probe_running = true;
    s_probe_thread = sim_thread_create(probe_thread, NULL);
    if (s_probe_thread == NULL)
    {
        s_probe_running = false;
        LOG_WARN("联网状态探测线程创建失败, /api/status/online 会一直返回未知状态");
        return;
    }

    LOG_DEBUG("联网状态探测已启动 (后台, %d 毫秒一轮)", PROBE_INTERVAL_MS);
}

void net_probe_stop(void)
{
    if (s_probe_thread == NULL) return;

    s_probe_running = false;
    int result_code = 0;
    sim_thread_join(s_probe_thread, &result_code);
    s_probe_thread = NULL;

    LOG_DEBUG("联网状态探测已停止");
}

network_status_t net_probe_status(void)
{
    return s_probe_cached;
}
