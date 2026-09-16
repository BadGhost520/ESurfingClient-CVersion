#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

static const char s_file_name[] = "run.log";
static const char s_rotate_file_name[] = ".rotate.log";

/**
 * @brief 单条日志的最大长度
 *
 * 多个进程共用同一个 run.log 时, 一条日志必须只用一次 write 写出,
 * 否则不同进程的行会互相穿插. 这里把上限定死并做静态检查
 */
#define LOG_LINE_MAX 2560

_Static_assert(LOG_LINE_MAX <= 4096, "日志行过长, 无法保证多进程下的原子写入");

/** @brief 文件身份复检间隔 (行), 用于发现日志文件已被其它进程轮转 */
#define LOG_ID_CHECK_LINES 32

/** @brief 本进程的日志轮转序号, 用于避免同一秒内的多次轮转重名 */
static uint32_t s_rotate_seq = 0;

static log_cfg_t s_logger_cfg = {
    .lv = LOG_LEVEL_INFO,
    .log_dir = "",
    .log_file = "",
    .file_handle = NULL,
    .max_lines = 1000,
    .cur_lines = 0,
    .file_dev = 0,
    .file_ino = 0,
    .lines_since_check = 0
};

/** @brief 是否同时把日志输出到控制台 */
static bool s_console_enabled = true;

static const char* get_level_str(const LogLevel lv)
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

/**
 * @brief 获取路径当前的文件身份
 *
 * 设备号 + inode 唯一标识一个文件, 重命名之后路径会指向新的 inode,
 * 据此可以判断自己手上的句柄是否已经落在被轮转掉的旧文件上
 * @param path 文件路径
 * @param dev 设备号 (Windows 为卷序列号)
 * @param ino inode (Windows 为文件索引)
 * @return 是否获取成功
 */
static bool get_file_id(const char* path, uint64_t* dev, uint64_t* ino)
{
#ifdef _WIN32
    HANDLE handle = CreateFileA(path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) return false;

    BY_HANDLE_FILE_INFORMATION info;
    const bool ok = GetFileInformationByHandle(handle, &info) != 0;
    CloseHandle(handle);
    if (ok == false) return false;

    *dev = (uint64_t)info.dwVolumeSerialNumber;
    *ino = ((uint64_t)info.nFileIndexHigh << 32) | (uint64_t)info.nFileIndexLow;
    return true;
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;

    *dev = (uint64_t)st.st_dev;
    *ino = (uint64_t)st.st_ino;
    return true;
#endif
}

/**
 * @brief 打开日志文件并记录它的身份
 * @return 是否打开成功
 */
static bool open_log_file()
{
    s_logger_cfg.file_handle = fopen(s_logger_cfg.log_file, "a");
    if (s_logger_cfg.file_handle == NULL) return false;

    s_logger_cfg.cur_lines = 0;
    s_logger_cfg.lines_since_check = 0;
    s_logger_cfg.file_dev = 0;
    s_logger_cfg.file_ino = 0;
    get_file_id(s_logger_cfg.log_file, &s_logger_cfg.file_dev, &s_logger_cfg.file_ino);
    return true;
}

/**
 * @brief 复检日志文件是否已被其它进程轮转
 *
 * 多个进程共用同一个 run.log, 任何一个进程都可能触发轮转,
 * 这里比对自身句柄与路径当前的 inode, 不一致就说明自己已经写到了旧文件上
 * @return 是否重新打开了日志文件
 */
static bool reopen_if_rotated()
{
    // 拿不到身份信息时无从比较, 退化为仅按自身行数计数
    if (!s_logger_cfg.file_handle || s_logger_cfg.file_ino == 0) return false;

    uint64_t dev = 0;
    uint64_t ino = 0;
    if (get_file_id(s_logger_cfg.log_file, &dev, &ino) == false) return false;
    if (dev == s_logger_cfg.file_dev && ino == s_logger_cfg.file_ino) return false;

    // 这里不能用 LOG_*, 会递归回到 log_out
    fprintf(stderr, "[INFO] 日志文件已被轮转, 重新打开: %s\n", s_logger_cfg.log_file);

    fclose(s_logger_cfg.file_handle);
    s_logger_cfg.file_handle = NULL;
    if (open_log_file() == false)
    {
        fprintf(stderr, "[ERROR] 轮转后无法重新打开日志文件 %s\n", s_logger_cfg.log_file);
        return false;
    }
    return true;
}

/**
 * @brief 日志轮转
 *
 * 轮转是"无主"的: 任何进程都可以发起, 并发安全由 rename 的原子性保证.
 * 同时发起时只有一个进程的 rename 会成功, 其余进程拿到失败,
 * 把失败直接当作"已被其它进程轮转"处理即可, 因此不需要锁或额外的协调
 */
static void rotate()
{
    if (!s_logger_cfg.file_handle || strlen(s_logger_cfg.log_file) == 0) return;

    if (s_logger_cfg.cur_lines < s_logger_cfg.max_lines)
    {
        // 未到阈值, 只按固定间隔复检一次身份, 避免每写一行都 stat
        if (s_logger_cfg.lines_since_check < LOG_ID_CHECK_LINES) return;
        s_logger_cfg.lines_since_check = 0;
        reopen_if_rotated();
        return;
    }

    /**
     * 到阈值了, 必须先确认自己没有写在旧文件上:
     * 否则会把别的进程刚轮转出来的新 run.log 再轮转一次
     */
    if (reopen_if_rotated() || s_logger_cfg.file_handle == NULL) return;

    char cur_tm[32];
    get_fmt_time(cur_tm, FILE_FORMAT);
    char rotate_file_name[PATH_MAX];
#ifdef _WIN32
    const unsigned long proc_id = (unsigned long)GetCurrentProcessId();
#else
    const unsigned long proc_id = (unsigned long)getpid();
#endif
    /**
     * 文件名带上进程号与本进程的轮转序号:
     * 时间戳只有秒级精度, 多个进程可能在同一秒内各自轮转,
     * 只靠时间戳必然重名, 而 rename 会覆盖同名目标, 那样一次就会丢掉一整份日志
     */
    const uint16_t result = snprintf(rotate_file_name, sizeof(rotate_file_name), "%s%c%s-%lu-%" PRIu32 "%s",
        safe_str(s_logger_cfg.log_dir), SEP, safe_str(cur_tm), proc_id, s_rotate_seq, s_rotate_file_name);
    s_rotate_seq++;
    if (result >= (uint16_t)sizeof(rotate_file_name))
    {
        fprintf(stderr, "[ERROR] 轮转的文件名过长 (最大 %zu)\n", sizeof(rotate_file_name) - 1);
        s_logger_cfg.cur_lines = 0;
        return;
    }

    // Windows 无法重命名一个仍被自己打开的文件, 所以先关闭再改名
    fclose(s_logger_cfg.file_handle);
    s_logger_cfg.file_handle = NULL;

    /**
     * 关闭之后再确认一次: 关闭与重命名之间仍有窗口, 期间可能已被其它进程轮转.
     * 不确认的话会把别的进程刚建好的新 run.log 改名走 (rename 会覆盖同名目标)
     */
    uint64_t dev = 0;
    uint64_t ino = 0;
    if (get_file_id(s_logger_cfg.log_file, &dev, &ino) == false ||
        dev != s_logger_cfg.file_dev || ino != s_logger_cfg.file_ino)
    {
        if (open_log_file() == false)
        {
            fprintf(stderr, "[ERROR] 轮转后无法重新打开日志文件 %s\n", s_logger_cfg.log_file);
        }
        return;
    }

    if (rename(s_logger_cfg.log_file, rotate_file_name) != 0)
    {
        // 多半是已被其它进程抢先轮转, 属于正常竞争
        fprintf(stderr, "[INFO] 日志轮转未生效 (可能已被其它进程轮转): %s\n", s_logger_cfg.log_file);
    }

    if (open_log_file() == false)
    {
        fprintf(stderr, "[ERROR] 轮转后无法重新打开日志文件 %s\n", s_logger_cfg.log_file);
    }
}

static bool get_log_dir(char* out)
{
#ifdef _WIN32
    char dir[PATH_MAX];
    if (get_exec_dir(dir) == false) return false;
    const uint16_t len = snprintf(out, PATH_MAX, "%s%clogs", safe_str(dir), SEP);
    if ((size_t)len >= PATH_MAX) return false;
    if (!CreateDirectoryA(out, NULL))
    {
        const DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) return false;
    }
#else
    const char dir[] = "/var/log/esurfing";
    const uint16_t len = snprintf(out, PATH_MAX, "%s%clogs", dir, SEP);
    if ((size_t)len >= PATH_MAX) return false;
    struct stat st;
    if (stat(out, &st) != 0)
    {
        if (mkdir("/var", 0755) != 0 && errno != EEXIST) return false;
        if (mkdir("/var/log", 0755) != 0 && errno != EEXIST) return false;
        if (mkdir(dir, 0755) != 0 && errno != EEXIST) return false;
        if (mkdir(out, 0755) != 0 && errno != EEXIST) return false;
    }
    else if (!S_ISDIR(st.st_mode)) return false;
#endif
    return true;
}

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
 * @brief 获取当前线程在日志里显示的标识
 *
 * 用配置序号而不是数组下标: 每个认证进程里数组下标都是 0,
 * 只有配置序号才能区分不同进程负责的账号
 * @return 标识字符串
 */
static char* get_thread_str()
{
    for (uint8_t i = 0; i < g_prog_cnt; i++)
    {
        if (sim_thread_cur_id() == g_prog_status[i].thread_id)
        {
            static char str[4];
            snprintf(str, sizeof(str), "%" PRIu8, g_prog_status[i].login_cfg.idx);
            return str;
        }
    }
    if (tl_thread_idx == -1)
    {
        return "Main";
    }
    return "WebServer";
}

void log_out(const LogLevel level, const char* file, const uint32_t line, const char* fmt, ...)
{
    if (level > s_logger_cfg.lv) return;
    if (!s_logger_cfg.file_handle)
    {
        fprintf(stderr, "[ERROR] 日志系统未打开, 无法输出日志\n");
        return;
    }
    va_list local_args;
    char ts[32];
    char msg[2048];
    char final_msg[LOG_LINE_MAX];
    get_fmt_time(ts, CONSOLE_FORMAT);
    va_start(local_args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, local_args);
    va_end(local_args);
    const int final_len = snprintf(final_msg, sizeof(final_msg),
        "[%s] [TID %" PRIu64 "] [T-%s] [%s] [%s:%d] %s\n",
        safe_str(ts),
        sim_thread_cur_id(),
        get_thread_str(),
        get_level_str(level),
        strrchr(file, '/') ? strrchr(file, '/') + 1 : strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file,
        line,
        safe_str(msg));
    if (final_len <= 0) return;

    // 被截断时按实际长度写出, 保证落到文件里的每一行都是完整的一行
    const size_t final_size = ((size_t)final_len < sizeof(final_msg)) ? (size_t)final_len : sizeof(final_msg) - 1;

    write_2_console(final_msg);
    write_2_file(final_msg, final_size);
    s_logger_cfg.cur_lines++;
    s_logger_cfg.lines_since_check++;
    rotate();
}

LogLevel get_logger_level()
{
    return s_logger_cfg.lv;
}

void set_logger_level(const LogLevel lv)
{
    if (s_logger_cfg.lv != lv)
    {
        s_logger_cfg.lv = lv;
        LOG_INFO("设置日志等级为 [%s]", get_level_str(lv));
    }
}

bool init_logger()
{
    if (get_log_dir(s_logger_cfg.log_dir) == false)
    {
        fprintf(stderr, "[ERROR] 无法准备日志目录\n");
        return false;
    }
    const uint16_t len = snprintf(s_logger_cfg.log_file, sizeof(s_logger_cfg.log_file), "%s%c%s", safe_str(s_logger_cfg.log_dir), SEP, s_file_name);
    if ((size_t)len >= sizeof(s_logger_cfg.log_file))
    {
        fprintf(stderr, "[ERROR] 日志文件路径太长 (最大 %zu)\n", sizeof(s_logger_cfg.log_file));
        return false;
    }
    if (open_log_file() == false)
    {
        fprintf(stderr, "[ERROR] 无法打开日志文件 %s, 如果是 Linux 系统请使用 sudo 运行程序\n", s_logger_cfg.log_file);
        return false;
    }
    LOG_DEBUG("日志系统初始化完成");
    LOG_DEBUG("日志等级: %s", get_level_str(s_logger_cfg.lv));
    return true;
}

void clean_logger()
{
    LOG_DEBUG("关闭日志系统");

    /**
     * 多进程下 run.log 是所有进程共用的, 退出时的重命名只能由一个进程来做:
     * 否则先退出的进程会把文件改名, 其它进程会继续往一个已改名的文件里写.
     * 因此认证/Web 进程只关闭自己的句柄, 改名交给守护进程 (或单进程模式)
     */
    const bool need_rename = (g_prog_role != ROLE_AUTH && g_prog_role != ROLE_WEB);

    if (!s_logger_cfg.file_handle)
    {
        fprintf(stderr, "[ERROR] 日志系统未启动\n");
        return;
    }
    fclose(s_logger_cfg.file_handle);
    s_logger_cfg.file_handle = NULL;

    if (need_rename == false) return;

    if (strlen(s_logger_cfg.log_file) == 0)
    {
        fprintf(stderr, "[ERROR] 日志路径为空\n");
        return;
    }
    char cur_tm[32];
    get_fmt_time(cur_tm, FILE_FORMAT);
    char new_file_name[PATH_MAX];
    snprintf(new_file_name, sizeof(new_file_name), "%s%c%s.log", safe_str(s_logger_cfg.log_dir), SEP, safe_str(cur_tm));
    rename(s_logger_cfg.log_file, new_file_name);
}

const char* get_logger_dir(void)
{
    return safe_str(s_logger_cfg.log_dir);
}

void set_logger_console(const bool enabled)
{
    s_console_enabled = enabled;
}
