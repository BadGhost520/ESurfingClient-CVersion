#include "utils/logger_internal.h"

static const char s_log_sub_dir[] = "logs";

#ifdef __OPENWRT__
static const char s_default_base[] = "/var/log/esurfing";
#endif

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
bool get_log_dir(char* out)
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

const char* get_logger_dir(void)
{
    return safe_str(s_logger_cfg.log_dir);
}

const char* get_logger_dir_cfg(void)
{
    // 配置里没写时回默认值, 页面上的输入框才不会空着
    return (s_cfg_log_dir[0] != '\0') ? s_cfg_log_dir : DEFAULT_LOG_DIR;
}
