#include "utils/LoggerInternal.h"

const char s_file_name[] = "run.log";

char s_cfg_log_dir[PATH_MAX] = "";

bool s_logger_inited = false;

#define LOG_LINE_MAX 2560

_Static_assert(LOG_LINE_MAX <= 4096, "日志行过长, 无法保证多进程下的原子写入");

log_cfg_t s_logger_cfg = {
    .lv = LOG_LEVEL_INFO,
    .log_dir = "",
    .log_file = "",
    .file_handle = NULL,
    .max_lines = 10000,
    .cur_lines = 0,
    .file_dev = 0,
    .file_ino = 0,
    .lines_since_check = 0
};

bool s_console_enabled = true;

bool s_query_mode = false;

const char* get_level_str(const LogLevel lv)
{
    switch (lv)
    {
    case LOG_LEVEL_VERBOSE: return "VERBOSE";
    case LOG_LEVEL_DEBUG:   return "DEBUG";
    case LOG_LEVEL_INFO:    return "INFO";
    case LOG_LEVEL_WARN:    return "WARN";
    case LOG_LEVEL_ERROR:   return "ERROR";
    case LOG_LEVEL_FATAL:   return "FATAL";
    default:                return "UNKNOWN";
    }
}

#ifdef _WIN32
static INIT_ONCE s_log_lock_once = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION s_log_lock;

static BOOL CALLBACK log_lock_init(PINIT_ONCE once, PVOID param, PVOID* ctx)
{
    (void)once;
    (void)param;
    (void)ctx;
    InitializeCriticalSection(&s_log_lock);
    return TRUE;
}

void logger_lock(void)
{
    InitOnceExecuteOnce(&s_log_lock_once, log_lock_init, NULL, NULL);
    EnterCriticalSection(&s_log_lock);
}

void logger_unlock(void)
{
    LeaveCriticalSection(&s_log_lock);
}
#else
static pthread_mutex_t s_log_lock = PTHREAD_MUTEX_INITIALIZER;

void logger_lock(void)
{
    pthread_mutex_lock(&s_log_lock);
}

void logger_unlock(void)
{
    pthread_mutex_unlock(&s_log_lock);
}
#endif

static void write_2_console(const char* msg)
{
    if (s_console_enabled == false) return;

    printf("%s", msg);
    fflush(stdout);
}

/**
 * @brief 写入日志文件
 *
 * 必须整行一次性写出:
 * run.log 以追加方式打开, 单次 write 会被内核对整个写入加锁,
 * 因此多个进程各自写完整的一行时不会互相穿插
 * @param msg 日志内容
 * @param len 日志长度
 */
static void write_2_file(const char* msg, const size_t len)
{
    if (!s_logger_cfg.file_handle || len == 0) return;

    /**
     * 句柄用 FILE* 保存, 但这里绕过 stdio 缓冲直接写 fd:
     * fprintf 可能拆成多次系统调用写出, 会破坏上面的原子性.
     * 因为从不通过这个 FILE* 做写入, 它的缓冲区始终为空, 混用是安全的
     */
#ifdef _WIN32
    const int fd = _fileno(s_logger_cfg.file_handle);
#else
    const int fd = fileno(s_logger_cfg.file_handle);
#endif
    if (fd < 0) return;

    size_t written = 0;
    while (written < len)
    {
#ifdef _WIN32
        const int n = _write(fd, msg + written, (unsigned int)(len - written));
#else
        const ssize_t n = write(fd, msg + written, len - written);
#endif
        if (n <= 0) return;
        written += (size_t)n;
    }
}

/**
 * @brief 不走常规日志流程, 直接往日志文件写一行
 *
 * 给看门狗用: 判定卡死时主线程可能正卡在某个调用里, 而常规的 log_out 会去
 * 判断轮转、可能还要建目录之类, 不该在那种状态下做。这里只做一次 write, 失败也不管。
 *
 * 格式与常规日志保持一致 (时间戳 / 进程标识 / 线程 / 级别 / 文件:行), 这样
 * 解析日志的工具不用为它开特例。
 * @param text 要写出的内容 (会自动补换行)
 */
void log_raw_line(const char* text)
{
    if (s_query_mode || !s_logger_cfg.file_handle) return;

    char ts[32];
    char proc_str[64];
    char line_buf[LOG_LINE_MAX];

    get_fmt_time(ts, CONSOLE_FORMAT);
    get_proc_str(proc_str, sizeof(proc_str));

    const int len = snprintf(line_buf, sizeof(line_buf),
        "[%s] [%s] [%s] [%s] [Watchdog.c:0] %s\n",
        safe_str(ts), proc_str, "watchdog", get_level_str(LOG_LEVEL_FATAL), safe_str(text));
    if (len <= 0) return;

    const size_t size = ((size_t)len < sizeof(line_buf)) ? (size_t)len : sizeof(line_buf) - 1;
    write_2_file(line_buf, size);
}

void log_out(const LogLevel level, const char* file, const uint32_t line, const char* fmt, ...)
{
    if (level > s_logger_cfg.lv) return;
    va_list local_args;
    char ts[32];
    char msg[2048];
    char final_msg[LOG_LINE_MAX];
    char proc_str[64];
    char thread_str[32];
    get_fmt_time(ts, CONSOLE_FORMAT);
    get_proc_str(proc_str, sizeof(proc_str));
    get_thread_str(thread_str, sizeof(thread_str));
    va_start(local_args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, local_args);
    va_end(local_args);
    const int final_len = snprintf(final_msg, sizeof(final_msg),
        "[%s] [%s] [%s] [%s] [%s:%d] %s\n",
        safe_str(ts),
        proc_str,
        thread_str,
        get_level_str(level),
        strrchr(file, '/') ? strrchr(file, '/') + 1 : strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file,
        line,
        safe_str(msg));
    if (final_len <= 0) return;

    // 被截断时按实际长度写出, 保证落到文件里的每一行都是完整的一行
    const size_t final_size = ((size_t)final_len < sizeof(final_msg)) ? (size_t)final_len : sizeof(final_msg) - 1;

    /**
     * 查询模式: 只写 stderr, 文件与 stdout 都不碰 (见 s_query_mode 的说明)
     *
     * 放在这里而不是函数开头: 上面那套等级过滤与格式化对两个去向是一样的,
     * 只有最后落在哪儿不同
     */
    if (s_query_mode)
    {
        fputs(final_msg, stderr);
        return;
    }

    write_2_console(final_msg);

    /**
     * 从这里到 rotate() 结束必须串行: 句柄的判空、取 fd、写、计数、轮转里的
     * fclose 都动同一份内存状态, 两个线程交叉就会 use-after-free (见上面互斥的说明)。
     * 格式化那一段放在锁外, 尽量缩短持锁时间。
     *
     * 判空也放在锁里: set_logger_dir() 换日志目录时会在持锁期间把句柄换掉
     * ("关旧的 -> 搬文件 -> 开新的" 没法拆开做), 在锁外判空会恰好撞上那一小段,
     * 把正常的一行日志误报成"日志系统未打开"
     */
    logger_lock();
    if (!s_logger_cfg.file_handle)
    {
        logger_unlock();
        fprintf(stderr, "[ERROR] 日志系统未打开, 无法输出日志\n");
        return;
    }
    write_2_file(final_msg, final_size);
    s_logger_cfg.cur_lines++;
    s_logger_cfg.lines_since_check++;
    rotate();
    logger_unlock();
}
