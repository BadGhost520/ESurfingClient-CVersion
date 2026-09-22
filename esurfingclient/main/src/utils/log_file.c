#include "utils/logger_internal.h"

static const char s_rotate_file_name[] = ".rotate.log";

#define LOG_ID_CHECK_LINES 32

static uint32_t s_rotate_seq = 0;

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
 * @brief 以追加方式打开一个日志文件
 * @param path 文件路径
 * @return 文件句柄 (失败返回 NULL)
 */
static FILE* open_log_handle(const char* path)
{
    FILE* handle = fopen(path, "a");
    if (handle == NULL) return NULL;

#ifndef _WIN32
    /**
     * 设成 exec 时自动关闭:
     * 监管者 fork 出子进程时会继承这里的句柄, 而子进程自己还要再开一份,
     * 不关掉的话子进程会一直白占着一个 fd
     * (Windows 侧用 CreateProcess 且不继承句柄, 无需处理)
     */
    const int fd = fileno(handle);
    if (fd >= 0)
    {
        fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
#endif

    return handle;
}

/**
 * @brief 打开日志文件并记录它的身份
 * @return 是否打开成功
 */
bool open_log_file()
{
    s_logger_cfg.file_handle = open_log_handle(s_logger_cfg.log_file);
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
void rotate()
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
