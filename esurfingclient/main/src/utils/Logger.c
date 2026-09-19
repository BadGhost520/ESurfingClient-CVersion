#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#include <io.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#ifndef EEXIST
#define EEXIST 17
#endif

static const char s_file_name[] = "run.log";
static const char s_rotate_file_name[] = ".rotate.log";

/** @brief 日志所在的子目录名 (基目录下的这一层) */
static const char s_log_sub_dir[] = "logs";

#ifdef __OPENWRT__
/**
 * @brief OpenWrt 上的默认日志基目录
 *
 * 配置里的 log_dir 在 OpenWrt 上同样生效 (想把日志放到 U 盘或别的分区上去时要用),
 * 没写时用这里的默认值, 于是默认仍旧是 /var/log/esurfing/logs。
 *
 * 默认选 /var/log 是因为它是 tmpfs: 重启即清, 也不磨损闪存, 小容量设备不会被日志占满 ——
 * 改配置把日志挪到闪存上的用户自己知道后果。
 *
 * ⚠️ init.d/esurfingclient.init 与 LuCI 的日志页都拿这个值兜底, 改这里要一起改
 */
static const char s_default_base[] = "/var/log/esurfing";
#endif

/**
 * @brief 配置文件里写的日志目录 (空字符串表示没写, 用默认值)
 *
 * 只存配置里的原样文本: 它是给页面回显用的, 不能拿解析后的绝对路径去回显,
 * 否则用户改一次配置就会被写成一长串绝对路径
 */
static char s_cfg_log_dir[PATH_MAX] = "";

/** @brief 日志系统是否已经初始化过 (决定 set_logger_dir 是"记下目录"还是"搬文件") */
static bool s_logger_inited = false;

/* ------------------------------------------------------------------
 * 进程内互斥
 *
 * 跨进程的原子性靠"一次 write 到 O_APPEND 的 fd", 那部分不需要锁。
 * 但 s_logger_cfg 这份【内存状态】在进程内是多线程共享的:
 * 一个线程在 write_2_file 里刚判完 file_handle 非空、正要 fileno(),
 * 另一个线程可能在 rotate() 里把它 fclose 掉了 —— 那就是 use-after-free,
 * 轻则崩溃, 重则把日志行写进一个已被复用的 fd (别人的 socket / 文件)。
 *
 * rotate() 不是罕见路径: 行数到阈值会走, 别的进程轮转过时也会走,
 * 而 OpenWrt 上多个账号共写同一份 run.log, 后者经常发生。
 *
 * ⚠️ log_raw_line() 【不能】用这把锁: 它是看门狗判定卡死时用的,
 *    那时主线程很可能正卡在持有本锁的调用里, 去抢锁等于一起卡住。
 *    它只做一次 write, 本来就够安全。
 * ------------------------------------------------------------------ */
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

static void logger_lock(void)
{
    InitOnceExecuteOnce(&s_log_lock_once, log_lock_init, NULL, NULL);
    EnterCriticalSection(&s_log_lock);
}

static void logger_unlock(void)
{
    LeaveCriticalSection(&s_log_lock);
}
#else
static pthread_mutex_t s_log_lock = PTHREAD_MUTEX_INITIALIZER;

static void logger_lock(void)
{
    pthread_mutex_lock(&s_log_lock);
}

static void logger_unlock(void)
{
    pthread_mutex_unlock(&s_log_lock);
}
#endif

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
static bool open_log_file()
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

/**
 * @brief 检查路径是不是一个目录
 * @param path 路径
 * @return 是否是目录
 */
static bool is_dir(const char* path)
{
#ifdef _WIN32
    const DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode) != 0;
#endif
}

#ifdef _WIN32

/**
 * @brief 跳过一个路径组件 (跳到它后面的分隔符之后)
 * @param p 当前位置
 * @return 下一个组件的位置 (已经到结尾时返回指向结尾的指针)
 */
static char* skip_component(char* p)
{
    while (*p != '\0' && *p != SEP) p++;
    if (*p == SEP) p++;
    return p;
}

/**
 * @brief 前缀比较 (Windows 上路径不分大小写)
 * @param text 文本
 * @param prefix 前缀
 * @return 是否以该前缀开头
 */
static bool prefix_eq(const char* text, const char* prefix)
{
    return _strnicmp(text, prefix, strlen(prefix)) == 0;
}

#endif // _WIN32

/**
 * @brief 逐级创建目录 (Posix 的 mkdir -p / Windows 的逐级 CreateDirectory)
 *
 * 配置里的日志目录可能有好几层都还不存在 (例如 /tmp/esurfing/logs/old),
 * 而 mkdir 只肯建最后一级, 所以这里自己逐级往下建
 * @param path 目录路径
 * @return 目录是否可用
 */
static bool make_dirs(const char* path)
{
    char buf[PATH_MAX];
    const int len = snprintf(buf, sizeof(buf), "%s", path);
    if (len <= 0 || (size_t)len >= sizeof(buf)) return false;

#ifdef _WIN32
    // 配置里可能写成 D:/esurfing/logs 这种混合分隔符, 先统一成 Windows 形式
    for (char* p = buf; *p != '\0'; p++)
    {
        if (*p == '/') *p = SEP;
    }

    /**
     * 先把"不能拿去建目录"的前缀跳过, 只把后面的部分逐级建出来
     *
     * 前缀有三种形状:
     *   D:\...                盘符
     *   \\服务器\共享名\...   UNC (\\ 与 \\服务器 这两段都不能单独建)
     *   \\?\D:\... \\?\UNC\…  扩展长度前缀 (路径超过 260 字符要用它, 这里同样支持)
     */
    char* p = buf;
    if (p[0] == SEP && p[1] == SEP)
    {
        p += 2;
        if (p[0] == '?' && p[1] == SEP)
        {
            p += 2;
            // \\?\UNC\服务器\共享名\... : UNC 那一段也不能单独建
            if (prefix_eq(p, "UNC") && p[3] == SEP)
            {
                p = skip_component(skip_component(p + 4));
            }
            // \\?\D:\... : 剩下的就是普通盘符路径
            if (isalpha((unsigned char)p[0]) != 0 && p[1] == ':') p += 2;
        }
        else
        {
            // \\服务器\共享名\... : 这两段由系统管着, 建不得
            p = skip_component(skip_component(p));
        }
    }
    else if (isalpha((unsigned char)p[0]) != 0 && p[1] == ':')
    {
        p += 2;
    }

    // 前缀后面紧跟的那个分隔符属于前缀, 从它【之后】的组件开始逐级建
    if (*p == SEP) p++;

    for (; *p != '\0'; p++)
    {
        if (*p != SEP) continue;
        *p = '\0';
        if (!CreateDirectoryA(buf, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
        *p = SEP;
    }
    if (!CreateDirectoryA(buf, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
#else
    for (char* p = buf + 1; *p != '\0'; p++)
    {
        if (*p != SEP) continue;
        *p = '\0';
        if (mkdir(buf, 0755) != 0 && errno != EEXIST) return false;
        *p = SEP;
    }
    if (mkdir(buf, 0755) != 0 && errno != EEXIST) return false;
#endif

    /**
     * 上面每一级都是"存在就跳过" (EEXIST / ERROR_ALREADY_EXISTS),
     * 而路径上摆着一个同名【文件】时同样会走到这里, 所以最后再确认一次是不是目录
     */
    return is_dir(buf);
}

/**
 * @brief 是否是绝对路径
 * @param path 路径
 * @return 是否绝对路径
 */
static bool is_abs_path(const char* path)
{
    if (path == NULL || path[0] == '\0') return false;
#ifdef _WIN32
    // \\服务器\共享名\... (UNC) 与 \dir 都算绝对路径
    if (path[0] == '/' || path[0] == '\\') return true;
    /**
     * 盘符形式只认 "D:\" / "D:/"。
     * "D:dir" 是 Windows 的"盘符相对"写法 (指 D 盘当前目录下的 dir), 而本程序的
     * 工作目录随环境变 (服务模式下可能是 System32), 按它解析没有意义, 因此不算绝对路径,
     * 后面会明确判为不可用 (见 resolve_log_dir)
     */
    return isalpha((unsigned char)path[0]) != 0 && path[1] == ':' &&
        (path[2] == '/' || path[2] == '\\');
#else
    return path[0] == '/';
#endif
}

/**
 * @brief 路径是否存在 (文件或目录都算)
 * @param path 路径
 * @return 是否存在
 */
static bool path_exists(const char* path)
{
#ifdef _WIN32
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(path, &st) == 0;
#endif
}

/**
 * @brief 去掉路径结尾多余的分隔符
 * @param path 路径 (原地修改)
 */
static void strip_tail_sep(char* path)
{
    size_t len = strlen(path);
    while (len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\'))
    {
#ifdef _WIN32
        // Windows 的 "D:\" 不能剥, 剥了就剩 "D:" (那表示"当前目录", 不是根)
        if (len == 3 && path[1] == ':') break;
#endif
        path[--len] = '\0';
    }
}

/**
 * @brief 把配置里的日志基目录解析成绝对路径
 *
 * 相对路径的基准按平台分:
 * - 桌面端是【程序所在目录】, 不能按当前工作目录: 把程序放进 /usr/local/bin 或注册成
 *   服务时工作目录是 / 或 System32, 按工作目录解析会把日志写到一个谁也想不到的地方 ——
 *   配置文件与网页文件都是按程序目录找的, 日志也该如此
 * - OpenWrt 上是 /var/log/esurfing: 那边的程序装在只读的 /usr/bin 里, 拿它当基准
 *   既写不了也没意义
 * @param cfg_dir 配置里的取值 (空字符串 / "." / "./" 都表示用默认值)
 * @param out 输出缓冲 (至少 PATH_MAX 字节)
 * @return 是否解析成功
 */
static bool resolve_log_dir(const char* cfg_dir, char* out)
{
    char base[PATH_MAX];

#ifdef __OPENWRT__
    if (snprintf(base, sizeof(base), "%s", s_default_base) >= (int)sizeof(base)) return false;
#else
    if (get_exec_dir(base) == false) return false;
#endif

    if (cfg_dir == NULL || cfg_dir[0] == '\0' || strcmp(cfg_dir, ".") == 0)
    {
        return snprintf(out, PATH_MAX, "%s", base) < PATH_MAX;
    }

#ifdef _WIN32
    /**
     * "D:dir" 这种盘符相对写法: 指的是 D 盘【当前目录】下的 dir, 而当前目录随环境变
     * (服务模式下可能是 System32), 按它解析等于把日志写到一个谁也想不到的地方。
     * 直接判为不可用, 让调用方退回默认目录并给出告警, 比默默写歪强
     */
    if (isalpha((unsigned char)cfg_dir[0]) != 0 && cfg_dir[1] == ':' &&
        cfg_dir[2] != '\0' && cfg_dir[2] != '/' && cfg_dir[2] != '\\')
    {
        LOG_WARN("log_dir (%s) 是盘符相对路径 (D:dir 指 D 盘当前目录), 请写成 D:\\dir 这样的绝对路径", cfg_dir);
        return false;
    }
#endif

    /**
     * 环境变量不做展开 (%TEMP% / $HOME 之类): 程序按字面把它当目录名建出来。
     * 提一句免得用户以为写 %TEMP% 就会落到临时目录里, 找日志时一头雾水
     */
    if (strchr(cfg_dir, '%') != NULL || strchr(cfg_dir, '$') != NULL)
    {
        LOG_WARN("log_dir (%s) 里的环境变量不会被展开, 会按普通目录名处理", cfg_dir);
    }

    if (is_abs_path(cfg_dir))
    {
        if (snprintf(out, PATH_MAX, "%s", cfg_dir) >= PATH_MAX) return false;
        strip_tail_sep(out);
        return true;
    }

    // 相对路径: 先去掉开头的 "./", 免得拼出 "/./" 这种路径
    const char* rel = cfg_dir;
    while (rel[0] == '.' && (rel[1] == '/' || rel[1] == '\\')) rel += 2;
    while (rel[0] == '/' || rel[0] == '\\') rel++;

    if (rel[0] == '\0')
    {
        return snprintf(out, PATH_MAX, "%s", base) < PATH_MAX;
    }

    const int len = snprintf(out, PATH_MAX, "%s%c%s", base, SEP, rel);
    if (len <= 0 || (size_t)len >= PATH_MAX) return false;
    strip_tail_sep(out);
    return true;
}

/**
 * @brief 取实际使用的日志目录 (目录不存在时会建出来)
 *
 * 规则: 配置里的 log_dir 是【基目录】, 日志放在它下面的 logs 里。
 * 桌面端的基目录默认是程序所在目录 (那里还放着程序本体 / 配置文件 / portal,
 * 直接往里写会把目录搅乱); OpenWrt 上默认是 /var/log/esurfing, 于是日志仍旧落在
 * /var/log/esurfing/logs。
 * @param out 输出缓冲 (至少 PATH_MAX 字节)
 * @return 目录是否可用
 */
static bool get_log_dir(char* out)
{
    char base[PATH_MAX];

    if (resolve_log_dir(s_cfg_log_dir, base) == false)
    {
        fprintf(stderr, "[ERROR] 无法解析日志目录: %s\n", safe_str(s_cfg_log_dir));
        return false;
    }

    const int len = snprintf(out, PATH_MAX, "%s%c%s", base, SEP, s_log_sub_dir);
    if (len <= 0 || (size_t)len >= PATH_MAX)
    {
        fprintf(stderr, "[ERROR] 日志目录路径太长: %s%c%s\n", base, SEP, s_log_sub_dir);
        return false;
    }

    if (make_dirs(out) == false)
    {
        fprintf(stderr, "[ERROR] 无法创建日志目录 %s\n", out);
        return false;
    }
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
 * @brief 组装日志里的"是谁写的"这一段: 角色 + 账号 + 进程号
 *
 * 多个进程共用同一份 run.log, 所以每行必须能看出是哪个进程写的。
 * 光有线程号不够 —— 线程号既不告诉你是谁, 在 Linux 上还是 pthread_t 强转出来的
 * 一个巨大地址 (形如 131239767045824), 拿它去 ps 里也对不上。
 * @param buf 输出缓冲
 * @param len 缓冲长度
 */
static void get_proc_str(char* buf, const size_t len)
{
#ifdef _WIN32
    const unsigned long proc_id = (unsigned long)GetCurrentProcessId();
#else
    const unsigned long proc_id = (unsigned long)getpid();
#endif

    const char* role;
    switch (g_prog_role)
    {
        case ROLE_AUTH:       role = "auth";       break;
        case ROLE_WEB:        role = "web";        break;
        case ROLE_SUPERVISOR: role = "supervisor"; break;
        default:              role = "standalone"; break;
    }

    /**
     * 认证进程标上账号序号: OpenWrt 上每个账号一个进程, 光看角色分不清是哪个。
     *
     * 用 g_prog_account (即 --account 的值) 而不是 g_prog_status[0].login_cfg.idx:
     * 两者必然相等 (load_cfg 就是按 idx == g_prog_account 挑的), 但 g_prog_status
     * 要等配置加载完才有值 —— 那样"仅加载配置 N"这一行反而标不出账号, 而它恰恰
     * 是最需要标明是哪个账号的一行
     */
    if (g_prog_role == ROLE_AUTH && g_prog_account != 0)
    {
        snprintf(buf, len, "%s#%" PRIu8 " pid=%lu", role, g_prog_account, proc_id);
    }
    else
    {
        snprintf(buf, len, "%s pid=%lu", role, proc_id);
    }
}

/**
 * @brief 组装日志里的"是哪个线程"这一段
 *
 * 优先用线程自己在入口处声明的名字; 单进程模式下每个账号一个认证线程,
 * 那几条线程靠线程 ID 反查配置序号来区分。
 * @param buf 输出缓冲
 * @param len 缓冲长度
 */
static void get_thread_str(char* buf, const size_t len)
{
    if (tl_thread_name != NULL)
    {
        snprintf(buf, len, "%s", tl_thread_name);
        return;
    }

    /**
     * 只有单进程模式才需要按线程 ID 反查:
     * 拆分模式下认证逻辑就在主线程上跑, 而 work_auth() 也把主线程登记成了
     * 配置 0 的线程 —— 不加这个限制的话主线程会被标成 auth#1, 名不副实
     */
    if (g_prog_role == ROLE_STANDALONE && g_prog_status != NULL)
    {
        for (uint8_t i = 0; i < g_prog_cnt; i++)
        {
            if (sim_thread_cur_id() == g_prog_status[i].thread_id)
            {
                snprintf(buf, len, "auth#%" PRIu8, g_prog_status[i].login_cfg.idx);
                return;
            }
        }
    }

    snprintf(buf, len, "main");
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
    if (!s_logger_cfg.file_handle) return;

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

bool set_logger_dir(const char* dir)
{
    // 配置里没写就是默认目录, 什么都不用做
    if (dir == NULL || dir[0] == '\0') return false;

    if (strlen(dir) >= PATH_MAX)
    {
        LOG_WARN("log_dir 过长 (最多 %d 个字符), 使用默认日志目录 (%s)", PATH_MAX - 1, DEFAULT_LOG_DIR);
        return false;
    }

    if (strcmp(dir, s_cfg_log_dir) == 0) return true;

    /**
     * 配置里的 log_dir 是【基目录】, 日志放在它下面的 logs 里
     */
    char new_base[PATH_MAX];
    char new_dir[PATH_MAX];
    char new_file[PATH_MAX];
    if (resolve_log_dir(dir, new_base) == false)
    {
        LOG_WARN("log_dir (%s) 不可用, 使用默认日志目录 (%s)", safe_str(dir), DEFAULT_LOG_DIR);
        return false;
    }
    const int dir_len = snprintf(new_dir, sizeof(new_dir), "%s%c%s", new_base, SEP, s_log_sub_dir);
    if (dir_len <= 0 || (size_t)dir_len >= sizeof(new_dir))
    {
        LOG_WARN("log_dir (%s) 过长, 使用默认日志目录 (%s)", safe_str(dir), DEFAULT_LOG_DIR);
        return false;
    }
    if (make_dirs(new_dir) == false)
    {
        LOG_WARN("log_dir (%s) 不可用, 使用默认日志目录 (%s)", safe_str(dir), DEFAULT_LOG_DIR);
        return false;
    }
    const int len = snprintf(new_file, sizeof(new_file), "%s%c%s", new_dir, SEP, s_file_name);
    if (len <= 0 || (size_t)len >= sizeof(new_file))
    {
        LOG_WARN("日志文件路径太长 (最大 %zu), 使用默认日志目录 (%s)", sizeof(new_file) - 1, DEFAULT_LOG_DIR);
        return false;
    }

    /**
     * 日志系统还没起来 (init_logger 之前调用): 记下目录就够了, init 的时候自然会用上。
     * 不特判的话这里会先把日志文件打开, init_logger 再打开一次 —— 白漏一个句柄
     */
    if (s_logger_inited == false)
    {
        snprintf(s_cfg_log_dir, sizeof(s_cfg_log_dir), "%s", dir);
        return true;
    }

    /**
     * 解析出来的目录与现在用的一致 (默认配置里的 "./" 就是程序所在目录,
     * 它的 logs 也正是当前在用的那个目录): 记下配置的原文就行, 不必把句柄
     * 关掉再打开一次 —— 那一下不但没必要, 换目录那一行日志也会跟着多出来
     */
    if (strcmp(new_dir, s_logger_cfg.log_dir) == 0)
    {
        snprintf(s_cfg_log_dir, sizeof(s_cfg_log_dir), "%s", dir);
        return true;
    }

    /**
     * 换目录这一段必须整体在锁里
     *
     * 中间有一小会儿 file_handle 是空的 (要先关掉旧文件才能改名, Windows 上
     * 更不能重命名一个还开着的文件)。不加锁的话别的线程正好在这一刻写日志,
     * 就会拿到"日志系统未打开"并丢掉那一行。持锁之后那些写日志的线程只是等
     * 一小会儿, 醒来看到的已经是新目录的句柄了
     *
     * ⚠️ 唯一绕开这把锁的是 log_raw_line() (看门狗卡死时用的), 它在换目录的这一
     *    瞬间可能读到已经被关掉的旧句柄 —— 与 rotate() 里那个窗口是同一类问题,
     *    那边同样是明知故犯 (见文件开头关于互斥的说明)
     */
    logger_lock();

    char old_dir[PATH_MAX];
    char old_file[PATH_MAX];
    snprintf(old_dir, sizeof(old_dir), "%s", s_logger_cfg.log_dir);
    snprintf(old_file, sizeof(old_file), "%s", s_logger_cfg.log_file);

    if (s_logger_cfg.file_handle != NULL)
    {
        fclose(s_logger_cfg.file_handle);
        s_logger_cfg.file_handle = NULL;
    }

    /**
     * 把已经写下的那几行一起搬过去
     *
     * 日志目录要等配置加载完才知道, 而配置加载之前就开始写日志了 (配置读错了
     * 更得有日志), 不搬的话启动那几行会留在默认目录里: 收尾改名只认当前这一份,
     * 那个文件就永远留在那儿了。
     *
     * 只在目标还不存在时才搬 —— rename 会覆盖同名文件, 而多进程下所有进程
     * 共写同一份 run.log, 覆盖就等于把别的进程的日志丢掉。
     * 搬不动也无所谓 (跨文件系统时 rename 会失败), 顶多启动那几行留在原地
     */
    const bool old_file_exists = (strlen(s_logger_cfg.log_file) > 0) && path_exists(s_logger_cfg.log_file);
    if (old_file_exists && path_exists(new_file) == false && strcmp(s_logger_cfg.log_file, new_file) != 0)
    {
        rename(s_logger_cfg.log_file, new_file);
    }

    snprintf(s_logger_cfg.log_dir, sizeof(s_logger_cfg.log_dir), "%s", new_dir);
    snprintf(s_logger_cfg.log_file, sizeof(s_logger_cfg.log_file), "%s", new_file);

    if (open_log_file() == false)
    {
        /**
         * 新目录打不开 (权限 / 磁盘满): 退回原来的目录继续写。
         * 上面的 rename 可能已经把旧文件搬走了, 那样这里会新建一个同名文件,
         * 已经写下的内容仍在搬走的那份里, 不会丢
         */
        snprintf(s_logger_cfg.log_dir, sizeof(s_logger_cfg.log_dir), "%s", old_dir);
        snprintf(s_logger_cfg.log_file, sizeof(s_logger_cfg.log_file), "%s", old_file);
        open_log_file();

        logger_unlock();
        LOG_WARN("无法打开日志文件 %s, 继续使用 %s", new_file, old_dir);
        return false;
    }

    logger_unlock();

    snprintf(s_cfg_log_dir, sizeof(s_cfg_log_dir), "%s", dir);

    LOG_INFO("日志目录已改为 %s", new_dir);
    return true;
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
    s_logger_inited = true;
    return true;
}

void clean_logger()
{
    LOG_DEBUG("关闭日志系统");

    /**
     * 多进程下 run.log 是所有进程共用的, 退出时的重命名只能由一个进程来做:
     * 否则先退出的进程会把文件改名, 其它进程会继续往一个已改名的文件里写.
     * 因此认证/Web 进程只关闭自己的句柄, 改名交给守护进程 (或单进程模式)
     *
     * ⚠️ OpenWrt 上【没有】守护进程: procd 只跑 --role auth 实例, 而
     *    --list-accounts 也刻意不调用本函数 —— 于是没人改名, run.log 会一直追加。
     *    那边由 init 脚本在启动前归档 (archive_previous_log), procd + init 脚本
     *    在 OpenWrt 上就是那个"守护进程"。改动这里时别忘了那一处。
     */
    const bool need_rename = (g_prog_role != ROLE_AUTH && g_prog_role != ROLE_WEB);

    /**
     * 关句柄这一段也要与写日志串行: 否则某个线程可能刚判完句柄非空,
     * 这里就把它 fclose 掉了 —— 与 rotate() 里那个窗口是同一类问题
     */
    logger_lock();

    if (!s_logger_cfg.file_handle)
    {
        logger_unlock();
        fprintf(stderr, "[ERROR] 日志系统未启动\n");
        return;
    }
    fclose(s_logger_cfg.file_handle);
    s_logger_cfg.file_handle = NULL;

    // 收尾之后句柄已经没了, 再 set_logger_dir 只该记下目录, 不该去搬文件
    s_logger_inited = false;

    if (need_rename == false)
    {
        logger_unlock();
        return;
    }

    if (strlen(s_logger_cfg.log_file) == 0)
    {
        logger_unlock();
        fprintf(stderr, "[ERROR] 日志路径为空\n");
        return;
    }
    char cur_tm[32];
    get_fmt_time(cur_tm, FILE_FORMAT);
    char new_file_name[PATH_MAX];
    snprintf(new_file_name, sizeof(new_file_name), "%s%c%s.log", safe_str(s_logger_cfg.log_dir), SEP, safe_str(cur_tm));
    rename(s_logger_cfg.log_file, new_file_name);

    logger_unlock();
}

const char* get_logger_dir(void)
{
    return safe_str(s_logger_cfg.log_dir);
}

const char* get_logger_dir_cfg(void)
{
    // 配置里没写时回默认值, 页面上的输入框才不会空着
    return (s_cfg_log_dir[0] != '\0') ? s_cfg_log_dir : DEFAULT_LOG_DIR;
}

void set_logger_console(const bool enabled)
{
    s_console_enabled = enabled;
}
