#include "net/NetTraffic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/log/Logger.h"
#include "utils/platform/PlatformUtils.h"
#include "utils/sim/SimThread.h"

#include "webserver/HotspotControl.h"

#include <cJSON/cJSON.h>

#ifdef _WIN32
#include <windows.h>
/** 1601-01-01 到 1970-01-01 的 100 纳秒数 (FILETIME 转 Unix 时间用) */
#define NET_TRAFFIC_EPOCH_DIFF 116444736000000000ULL
typedef CRITICAL_SECTION traffic_lock_t;
#else
#include <pthread.h>
#include <time.h>
typedef pthread_mutex_t traffic_lock_t;
#endif

/** 最多同时跟踪多少块网卡 (家用/办公/路由都远到不了这个数) */
#define NET_TRAFFIC_MAX 32
/** 采样间隔 */
#define NET_TRAFFIC_INTERVAL_MS 1000
/** 睡眠切片: 停下时要能在一秒内响应 */
#define NET_TRAFFIC_SLICE_MS 100

/** 上一帧的计数器, 用来做差 */
typedef struct
{
    char id[ADAPTER_ID_LEN];
    uint64_t rx_bytes;
    uint64_t tx_bytes;
} traffic_prev_t;

/** 一帧对外的采样结果 */
typedef struct
{
    char id[ADAPTER_ID_LEN];
    char name[NAME_LENGTH];
    bool connected;
    bool has_rate;
    double down;
    double up;
} traffic_row_t;

static traffic_lock_t s_lock;
static bool s_lock_ready;
static volatile bool s_running;
static sim_thread_t* s_thread;

static traffic_row_t s_rows[NET_TRAFFIC_MAX];
static uint8_t s_row_count;
static char s_default_id[ADAPTER_ID_LEN];
static uint64_t s_timestamp_ms;

static traffic_prev_t s_prev[NET_TRAFFIC_MAX];
static uint8_t s_prev_count;
static uint64_t s_prev_at_ms;

/**
 * 平台相关的锁 / 时钟 / 睡眠收在这几个小函数里, 上面的采样逻辑就与平台无关
 */
static void traffic_lock_init(void)
{
#ifdef _WIN32
    InitializeCriticalSection(&s_lock);
#else
    pthread_mutex_init(&s_lock, NULL);
#endif
    s_lock_ready = true;
}

static void traffic_lock(void)
{
#ifdef _WIN32
    EnterCriticalSection(&s_lock);
#else
    pthread_mutex_lock(&s_lock);
#endif
}

static void traffic_unlock(void)
{
#ifdef _WIN32
    LeaveCriticalSection(&s_lock);
#else
    pthread_mutex_unlock(&s_lock);
#endif
}

/**
 * @brief 当前时间 (Unix 毫秒)
 *
 * 网页用它判断数据新不新鲜, 必须是墙上时钟而不是开机以来的毫秒数
 */
static uint64_t traffic_now_ms(void)
{
#ifdef _WIN32
    FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);
    ULARGE_INTEGER value;
    value.LowPart = file_time.dwLowDateTime;
    value.HighPart = file_time.dwHighDateTime;
    return (uint64_t)((value.QuadPart - NET_TRAFFIC_EPOCH_DIFF) / 10000ULL);
#else
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) return 0;
    return (uint64_t)now.tv_sec * 1000ULL + (uint64_t)now.tv_nsec / 1000000ULL;
#endif
}

static void traffic_sleep_ms(const int ms)
{
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec delay;
    delay.tv_sec = ms / 1000;
    delay.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&delay, NULL);
#endif
}

/**
 * @brief 采一帧: 向系统要计数器, 与上一帧做差得到 B/s
 *
 * 三种情况不给速率 (界面上显示"正在采样"):
 * 第一次见到这块网卡、与上一帧间隔为零、计数器变小 (网卡重插或清零)
 */
static void traffic_sample(void)
{
    adapter_stats_t frames[NET_TRAFFIC_MAX];
    char default_id[ADAPTER_ID_LEN] = {0};
    const uint8_t count = get_adapters_stats(frames, NET_TRAFFIC_MAX, default_id, sizeof(default_id));
    const uint64_t now = traffic_now_ms();

    traffic_row_t rows[NET_TRAFFIC_MAX];
    traffic_prev_t prev[NET_TRAFFIC_MAX];
    for (uint8_t i = 0; i < count; i++)
    {
        memset(&rows[i], 0, sizeof(rows[i]));
        // 两边都是定长数组, 直接按长度拷贝再补 NUL, 免得 snprintf 报"可能截断"
        memcpy(rows[i].id, frames[i].id, sizeof(rows[i].id));
        rows[i].id[sizeof(rows[i].id) - 1] = '\0';
        memcpy(rows[i].name, frames[i].name, sizeof(rows[i].name));
        rows[i].name[sizeof(rows[i].name) - 1] = '\0';
        rows[i].connected = frames[i].connected;
        memcpy(prev[i].id, frames[i].id, sizeof(prev[i].id));
        prev[i].id[sizeof(prev[i].id) - 1] = '\0';
        prev[i].rx_bytes = frames[i].rx_bytes;
        prev[i].tx_bytes = frames[i].tx_bytes;

        for (uint8_t j = 0; j < s_prev_count; j++)
        {
            if (strcmp(s_prev[j].id, frames[i].id) != 0) continue;
            const uint64_t elapsed = (s_prev_at_ms != 0 && now > s_prev_at_ms) ? (now - s_prev_at_ms) : 0;
            if (elapsed > 0 && frames[i].rx_bytes >= s_prev[j].rx_bytes && frames[i].tx_bytes >= s_prev[j].tx_bytes)
            {
                rows[i].has_rate = true;
                rows[i].down = (double)(frames[i].rx_bytes - s_prev[j].rx_bytes) * 1000.0 / (double)elapsed;
                rows[i].up = (double)(frames[i].tx_bytes - s_prev[j].tx_bytes) * 1000.0 / (double)elapsed;
            }
            break;
        }
    }

    traffic_lock();
    memcpy(s_rows, rows, sizeof(traffic_row_t) * count);
    s_row_count = count;
    snprintf(s_default_id, sizeof(s_default_id), "%s", default_id);
    s_timestamp_ms = now;
    memcpy(s_prev, prev, sizeof(traffic_prev_t) * count);
    s_prev_count = count;
    s_prev_at_ms = now;
    traffic_unlock();
}

static int traffic_thread(void* arg)
{
    (void)arg;
    while (s_running)
    {
        traffic_sample();
        for (int i = 0; i < NET_TRAFFIC_INTERVAL_MS / NET_TRAFFIC_SLICE_MS; i++)
        {
            if (s_running == false) return 0;
            traffic_sleep_ms(NET_TRAFFIC_SLICE_MS);
        }
    }
    return 0;
}

bool net_traffic_start(void)
{
    if (s_thread != NULL) return true;
    if (s_lock_ready == false) traffic_lock_init();

    traffic_sample(); // 先采一帧: 只建立基线, 还没有速率
    s_running = true;
    s_thread = sim_thread_create(traffic_thread, NULL);
    if (s_thread == NULL)
    {
        s_running = false;
        LOG_WARN("网速采样线程创建失败");
        return false;
    }
    LOG_INFO("网速采样已启动 (每秒一帧)");
    return true;
}

void net_traffic_stop(void)
{
    if (s_thread == NULL) return;
    s_running = false;
    int code = 0;
    sim_thread_join(s_thread, &code);
    s_thread = NULL;
    LOG_INFO("网速采样已停止");
}

char* net_traffic_json(void)
{
    if (s_lock_ready == false || s_row_count == 0) return NULL;

    cJSON* root = cJSON_CreateObject();
    if (root == NULL) return NULL;

    traffic_lock();
    cJSON_AddNumberToObject(root, "timestamp", (double)s_timestamp_ms);
    cJSON_AddStringToObject(root, "defaultId", s_default_id);
    cJSON* adapters = cJSON_CreateArray();
    for (uint8_t i = 0; i < s_row_count; i++)
    {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "id", s_rows[i].id);
        cJSON_AddStringToObject(item, "name", s_rows[i].name);
        cJSON_AddBoolToObject(item, "connected", s_rows[i].connected);
        if (s_rows[i].has_rate)
        {
            cJSON_AddNumberToObject(item, "down", s_rows[i].down);
            cJSON_AddNumberToObject(item, "up", s_rows[i].up);
        }
        else
        {
            // 首帧没有速率: 给 null, 界面显示"正在采样"
            cJSON_AddNullToObject(item, "down");
            cJSON_AddNullToObject(item, "up");
        }
        cJSON_AddItemToArray(adapters, item);
    }
    traffic_unlock();

    cJSON_AddItemToObject(root, "adapters", adapters);
    // 热点卡片是否显示, 由"这个平台能不能控制热点"决定 (目前只有 Windows 能)
    cJSON_AddBoolToObject(root, "hotspotControls", hotspot_control_available());

    char* json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}
