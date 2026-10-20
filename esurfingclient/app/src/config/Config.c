#include "utils/platform/PlatformUtils.h"
#include "utils/text/TextUtils.h"
#include "utils/time/TimeUtils.h"

#include "core/States.h"

#include "config/Config.h"

#include "ConfigInternal.h"

#include "utils/log/Logger.h"

#include "cJSON/cJSON.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __OPENWRT__
static const char config_file[] = "/etc/config/esurfingclient";
#else
#define DIALER_CONFIG_FILE "ESurfingClient.json"
static char config_file[PATH_MAX + 1 + sizeof(DIALER_CONFIG_FILE)];
#endif

static const char s_default_cfg[] = "{\n"
                                    "   \"enabled\": false,\n"
                                    "   \"web_external_acc\": false,\n"
                                    "   \"log_lv\": 4,\n"
                                    "   \"log_dir\": \"./\",\n"
                                    "   \"conn_timeout\": 7,\n"
                                    "   \"op_timeout\": 10,\n"
                                    "   \"web_port\": 8888,\n"
                                    "   \"accounts\": [\n"
                                    "       {\n"
                                    "           \"username\": \"\",\n"
                                    "           \"password\": \"\",\n"
                                    "           \"channel\": 3,\n"
                                    "           \"mark\": \"\",\n"
                                    "           \"time_windows\": []\n"
                                    "       }\n"
                                    "   ]\n"
                                    "}\n";

/**
 * @brief 读一个整数字段, 缺失/类型不对就用默认值 (并打一行日志)
 */
static long cfg_read_num(const cJSON* obj, const char* name, const long def)
{
    const cJSON* item = cJSON_GetObjectItem(obj, name);
    if (item == NULL)
    {
        LOG_DEBUG("%s 参数不存在, 使用默认值 %ld", name, def);
        return def;
    }
    if (cJSON_IsNumber(item) == false)
    {
        LOG_WARN("%s 参数不正确, 使用默认值 %ld", name, def);
        return def;
    }
    return item->valueint;
}

/**
 * @brief 读一个布尔字段, 缺失/类型不对就用默认值 (并打一行日志)
 */
static bool cfg_read_bool(const cJSON* obj, const char* name, const bool def)
{
    const cJSON* item = cJSON_GetObjectItem(obj, name);
    if (item == NULL)
    {
        LOG_DEBUG("%s 参数不存在, 使用默认值 %s", name, def ? "true" : "false");
        return def;
    }
    if (cJSON_IsBool(item) == false)
    {
        LOG_WARN("%s 参数不正确 (应为 true / false), 使用默认值 %s", name, def ? "true" : "false");
        return def;
    }
    return cJSON_IsTrue(item);
}

/**
 * @brief 读一个字符串字段 (返回 NULL 表示缺失或类型不对)
 */
static const char* cfg_read_str(const cJSON* obj, const char* name)
{
    const cJSON* item = cJSON_GetObjectItem(obj, name);
    if (item == NULL || cJSON_IsString(item) == false || item->valuestring == NULL) return NULL;
    return item->valuestring;
}

static bool write_cfg_json(const cJSON* cfg_json)
{
    char* cfg_text = cJSON_Print(cfg_json);
    if (cfg_text == NULL)
    {
        LOG_ERROR("配置内容序列化失败");
        return false;
    }

    FILE* cfg_file = fopen(config_file, "w");
    if (!cfg_file)
    {
        LOG_ERROR("无法生成文件: %s", config_file);
        free(cfg_text);
        return false;
    }
    fprintf(cfg_file, "%s", cfg_text);
    fclose(cfg_file);

    free(cfg_text);
    return true;
}

#ifndef __OPENWRT__

bool save_cfg(const char* configs_str)
{
    LOG_INFO("保存配置中");
    LOG_INFO("仅会保存第一个可用配置");

    cJSON* configs = cJSON_Parse(configs_str);
    if (!configs)
    {
        LOG_ERROR("配置 JSON 解析失败");
        return false;
    }

    const bool saved = write_cfg_json(configs);
    cJSON_Delete(configs);

    return saved;
}

const char* get_config_file_path(void)
{
    return config_file;
}

#endif

bool s_list_only = false;

const char* get_config_path(void)
{
    return config_file;
}

static uint64_t s_cfg_hash = 0;

static bool s_cfg_waiting = false;

#define CFG_WAIT_TICK_MS 1000

/**
 * @brief 读取配置文件的原文
 *
 * 程序还没把配置读起来时 (停在"等配置"状态, 或者网页刚保存、重新加载还没跑完),
 * 网页直接展示文件里的内容: 内存里那份是空的或者旧的, 显示成空账号的话,
 * 用户一保存就把自己写过的账号密码覆盖掉了
 * @return 文件内容 (需要 free), 读不到或文件为空时返回 NULL
 */
char* read_cfg_file()
{
    FILE* file = fopen(config_file, "rb");
    if (file == NULL) return NULL;

    fseek(file, 0, SEEK_END);
    const long len = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (len <= 0)
    {
        fclose(file);
        return NULL;
    }

    char* text = malloc((size_t)len + 1);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }

    const size_t read_len = fread(text, 1, (size_t)len, file);
    fclose(file);
    text[read_len] = '\0';

    return text;
}

/**
 * @brief 取配置文件内容的指纹 (FNV-1a)
 *
 * 等配置的程序靠它判断配置文件有没有被改过: 没改就不必重新解析 (解析一次要写好几行
 * 日志, 一秒一次的话等一晚上日志就炸了)。
 *
 * 看内容而不是修改时间, 有两个原因:
 * - 修改时间在有些文件系统上只精确到秒 (FAT32 之类), 同一秒里改完保存会被漏掉,
 *   程序就一直等下去了
 * - 网页保存与手工编辑两条路都不用额外发信号, 内容变了就一定会被发现
 * 配置文件只有几百字节, 一秒读一次的开销可以忽略
 * @return 指纹值 (文件不存在或读不到时为 0)
 */
uint64_t cfg_file_hash()
{
    FILE* file = fopen(config_file, "rb");
    if (file == NULL) return 0;

    uint64_t hash = 14695981039346656037ull;
    int ch;
    while ((ch = fgetc(file)) != EOF)
    {
        hash ^= (uint64_t)(uint8_t)ch;
        hash *= 1099511628211ull;
    }
    fclose(file);

    // 0 留给"读不到", 免得空文件与打不开撞在一起
    return hash == 0 ? 1 : hash;
}

/**
 * @brief 等配置文件变成可用 (每次最多等一个固定的间隔)
 *
 * 配置没变就不重新解析: 解析一次要写好几行日志, 一秒一次的话等一晚上日志就炸了。
 * 网页上的"保存并应用"会置 g_need_reload, 那条路不必等下一次轮询
 * @return true 表示配置已可用 (此时 load_cfg 已经跑完)
 */
bool wait_cfg_usable(void)
{
    /**
     * 进入"等配置"状态时先记下配置文件的指纹:
     * 之后只有它变了 (网页保存 / 手工编辑 / 点了"保存并应用") 才重新解析
     */
    if (s_cfg_waiting == false)
    {
        s_cfg_waiting = true;
        s_cfg_hash = cfg_file_hash();
        g_need_reload = false;
    }

    sleep_ms(CFG_WAIT_TICK_MS, false);

    const uint64_t hash = cfg_file_hash();
    if (g_need_reload == false && hash == s_cfg_hash) return false;

    g_need_reload = false;
    s_cfg_hash = hash;

    if (load_cfg() == true)
    {
        s_cfg_waiting = false;
        LOG_INFO("配置已可用, 继续往下走");
        return true;
    }

    LOG_WARN("配置仍不可用, 请在网页界面上改好后再点「保存并应用」");
    return false;
}

/** 一个账号的解析结果 */
typedef enum
{
    ACCOUNT_READY = 0,  // 填好了, 可用
    ACCOUNT_SKIP,       // 账号或密码没填 → 跳过这个账号
    ACCOUNT_INVALID     // time_windows 非法 (整份配置都算不可用)
} account_state_t;

/**
 * @brief 把一个账号对象里能填的填进 cfg, 并校验必填项
 *
 * OpenWrt 与桌面两条分支原本各写了一遍这段 (取字段 / 校验账号密码 / 应用信道
 * 与 UA / 解析时间窗口), 日志文案都一字不差。这里只抽"填 + 校验",
 * 至于放进哪个槽位、标记值怎么分配、失败之后是退出还是标记不可用, 仍由各自循环决定。
 *
 * @param account 账号对象
 * @param cfg_no 配置序号 (从 1 开始, 只用于日志)
 * @param cfg 输出
 * @return 解析结果
 */
static account_state_t fill_account(const cJSON* account, const uint8_t cfg_no, login_cfg_t* cfg)
{
    const cJSON* usr = cJSON_GetObjectItem(account, "username");
    const cJSON* pwd = cJSON_GetObjectItem(account, "password");
    const cJSON* chn = cJSON_GetObjectItem(account, "channel");

    // 先把用户写过的东西按原样填进去: 配置不可用时网页要靠这块内存回显
    cfg->idx = cfg_no;
    cfg->chn = parse_channel_json(chn, cfg_no);
    apply_channel_ua(cfg, cfg_no);
    snprintf(cfg->usr, USR_LEN, "%s", safe_str(usr != NULL ? usr->valuestring : ""));
    snprintf(cfg->pwd, PWD_LEN, "%s", safe_str(pwd != NULL ? pwd->valuestring : ""));
    LOG_DEBUG("配置 %" PRIu8 " 使用 UA: %s", cfg_no, cfg->user_agent);

    if (usr == NULL || cJSON_IsString(usr) == false || usr->valuestring[0] == '\0')
    {
        LOG_WARN("配置 %" PRIu8 " username 参数缺失或为空, 跳过当前配置", cfg_no);
        return ACCOUNT_SKIP;
    }
    if (pwd == NULL || cJSON_IsString(pwd) == false || pwd->valuestring[0] == '\0')
    {
        LOG_WARN("配置 %" PRIu8 " password 参数缺失或为空, 跳过当前配置", cfg_no);
        return ACCOUNT_SKIP;
    }

    if (apply_time_windows(cJSON_GetObjectItem(account, "time_windows"), cfg) == false)
    {
        LOG_FATAL("配置 %" PRIu8 " time_windows 非法, 应为 [{ \"start\": \"mon 08:13\", \"end\": \"mon 23:57\" }, ...]", cfg_no);
        return ACCOUNT_INVALID;
    }

    return ACCOUNT_READY;
}

bool load_cfg()
{
    g_cfg_loaded = false;
    /**
     * 桌面分支直接写 g_prog_status[0], 这里保证至少有一格可用
     * (OpenWrt 分支后面会按配置数重新分配)
     */
    if (g_prog_status == NULL)
    {
        g_prog_status = calloc(1, sizeof(prog_status_t));
        if (g_prog_status == NULL)
        {
            LOG_FATAL("分配内存失败");
            return false;
        }
    }
#ifndef __OPENWRT__

    char dir[PATH_MAX];
    if (get_exec_dir(dir) == false)
    {
        LOG_ERROR("获取可执行文件路径失败, 请检查权限后重启");
        return false;
    }
    snprintf(config_file, PATH_MAX + 1 + sizeof(DIALER_CONFIG_FILE), "%s%c%s", safe_str(dir), SEP, DIALER_CONFIG_FILE);

#endif

    FILE* cfg_file = fopen(config_file, "r");
    if (!cfg_file || fgetc(cfg_file) == EOF)
    {
        LOG_ERROR("无法打开配置文件或配置文件为空: %s", config_file);
        LOG_INFO("创建新的默认配置文件");
        FILE* new_cfg = fopen(config_file, "w");
        if (!new_cfg)
        {
            LOG_FATAL("无法生成文件: %s, 请检查权限后重启", config_file);
            return false;
        }
        fprintf(new_cfg, "%s", s_default_cfg);
        fclose(new_cfg);
        LOG_INFO("创建完成, 请在 %s 填写账号数据后重启", config_file);
        return false;
    }

    fseek(cfg_file, 0, SEEK_END);
    const long len = ftell(cfg_file);
    fseek(cfg_file, 0, SEEK_SET);

    if (len < 0)
    {
        LOG_ERROR("读取配置文件失败: %s", config_file);
        fclose(cfg_file);
        return false;
    }

    char* cfg_data = malloc((size_t)len + 1);
    if (cfg_data == NULL)
    {
        LOG_FATAL("分配内存失败");
        fclose(cfg_file);
        return false;
    }
    const size_t read_len = fread(cfg_data, 1, (size_t)len, cfg_file); // 短读时按实际长度截断
    cfg_data[read_len] = '\0';
    fclose(cfg_file);

    cJSON* cfg_json = cJSON_Parse(cfg_data);
    free(cfg_data);
    if (!cfg_json)
    {
        LOG_FATAL("JSON 解析失败, 请检查后重启");
        return false;
    }

    /**
     * 先把配置补全, 再往下解析
     *
     * 放在这里是为了后面每一处解析都能拿到完整的对象; 补全后的内容要写回文件,
     * 用户下次打开配置就能看见自己漏了什么 (写不回去也不影响本次运行, 值已经在内存里)
     */
    const uint8_t completed = complete_cfg(cfg_json);
    if (completed > 0)
    {
        LOG_INFO("配置文件缺少参数, 已按默认值补全 %" PRIu8 " 个", completed);
        if (write_cfg_json(cfg_json))
        {
            LOG_INFO("补全后的配置已写回 %s", config_file);
        }
        else
        {
            LOG_WARN("补全后的配置写回失败 (配置文件可能是只读的?), 本次仍按补全后的值运行");
        }
    }

    /**
     * Web 服务端口与是否允许外部访问
     *
     * 只有桌面端有 Web 服务 (OpenWrt 版不带 Web 服务), 那边这两个参数
     * 解析出来也没人用, 但配置文件的格式是同一套, 这里就一起读掉。
     *
     * 必须排在下面所有"配置不可用"的判断之前: 配置被禁用 / 账号没填好时,
     * 程序会带着网页停在"等配置"的状态, 网页要按配置里的端口监听 ——
     * 排在后面的话就只能按默认端口开出去了
     */
    const long web_port = cfg_read_num(cfg_json, "web_port", DEFAULT_WEB_PORT);
    g_web_port = (web_port >= 1 && web_port <= 65535) ? (uint16_t)web_port : DEFAULT_WEB_PORT;
    g_web_external_acc = cfg_read_bool(cfg_json, "web_external_acc", DEFAULT_WEB_EXTERNAL_ACC);

    /**
     * 从这里开始"尽力解析": 配置不可用 (被禁用 / 账号没填全) 时不再半路返回,
     * 而是把能读出来的值照样填进内存, 只用 usable 记下它不可用。
     *
     * 之所以要这样: 配置不可用时程序会带着网页停下来等用户改, 而配置页读的
     * 就是这块内存 —— 空着的话用户在网页上点一次保存, 就把自己写过的账号密码
     * 覆盖成空的了
     */
    bool usable = true;

    /**
     * 下面这些"参数不存在"的分支现在基本走不到了 (上面已经补全),
     * 留着是给补全时分配内存失败之类的极端情况兜底
     */
    const cJSON* enabled = cJSON_GetObjectItem(cfg_json, "enabled");
    if (enabled == NULL)
    {
        LOG_WARN("enabled 参数不存在, 请填写后重启程序");
        g_prog_enabled = false;
        usable = false;
    }
    else if (cJSON_IsFalse(enabled))
    {
        LOG_WARN("配置文件中禁用了程序启动, 请开启后重启程序");
        g_prog_enabled = false;
        usable = false;
    }
    else
    {
        g_prog_enabled = true;
    }

    set_logger_level((LogLevel)cfg_read_num(cfg_json, "log_lv", LOG_LEVEL_INFO));

    /**
     * 日志目录
     *
     * 配置里的 log_dir 是【基目录】, 日志放在它下面的 logs 里 (默认就是程序所在目录下的
     * logs, OpenWrt 上则固定为 /var/log/esurfing/logs)。
     * 必须在这里才能定下来: 日志系统是先起来再读配置的 (配置读错了更要有日志),
     * set_logger_dir 会把已经写下的那几行一起搬到新目录去。
     * OpenWrt 上基目录是写死的, 那边只有写了别的目录才会提一句。
     */
    const char* log_dir = cfg_read_str(cfg_json, "log_dir");
    if (log_dir == NULL)
    {
        LOG_DEBUG("log_dir 参数不存在, 使用默认值 (%s)", get_logger_dir_cfg());
    }
    else
    {
        set_logger_dir(log_dir);
    }

    g_conn_timeout = cfg_read_num(cfg_json, "conn_timeout", DEFAULT_CONN_TIMEOUT);
    g_op_timeout = cfg_read_num(cfg_json, "op_timeout", DEFAULT_OP_TIMEOUT);

    const cJSON* accounts = cJSON_GetObjectItem(cfg_json, "accounts");

    /**
     * 账号整段缺失 / 是空的 / 根本不是数组: 记下不可用, 但不再半路返回 ——
     * 后面按"一个可用账号都没有"处理, 网页上还能把账号补上
     */
    if (accounts == NULL || cJSON_IsArray(accounts) == false || cJSON_GetArraySize(accounts) == 0)
    {
        LOG_FATAL("没有找到账号数据, 请添加后重启程序");
        usable = false;
    }

    const uint8_t cnt = cJSON_GetArraySize(accounts);

    int8_t valid_cnt = 0;

#ifdef __OPENWRT__
    if (g_prog_account != 0)
    {
        LOG_INFO("OpenWRT 环境, 本次仅使用配置 %" PRIu8, g_prog_account);
    }
    else
    {
        LOG_INFO("OpenWRT 环境, 会尝试加载所有有效配置");
    }

    prog_status_t* new_prog_status = realloc(g_prog_status, sizeof(prog_status_t) * cnt);
    if (new_prog_status)
    {
        g_prog_status = new_prog_status;
        memset(g_prog_status, 0, sizeof(prog_status_t) * cnt);
    }
    else
    {
        LOG_FATAL("重分配内存失败");
        return false;
    }

    bool use_cus_mark = false;

    for (uint8_t i = 0, valid_i = 0; i < cnt; i++)
    {
        const cJSON* account = cJSON_GetArrayItem(accounts, i);
        const cJSON* mark = cJSON_GetObjectItem(account, "mark");

        login_cfg_t* login = &g_prog_status[valid_i].login_cfg;
        const account_state_t state = fill_account(account, i + 1, login);
        if (state == ACCOUNT_SKIP) continue;
        if (state == ACCOUNT_INVALID)
        {
            cJSON_Delete(cfg_json);
            return false;
        }

        // 标记值按"放进哪个槽位"分配, 所以留在循环里
        if (mark == NULL)
        {
            if (use_cus_mark)
            {
                LOG_WARN("其它配置使用了自定义标记值, 但配置 %" PRIu8 " 未填写, 将跳过该配置", i + 1);
                continue;
            }
            login->mark = 0x100 + valid_i * 0x100;
            LOG_DEBUG("使用自动标记值: %" PRIu32 " (0x%x)", login->mark, login->mark);
            LOG_DEBUG("当前使用下标: %" PRIu8, valid_i);
        }
        else
        {
            if (use_cus_mark && mark->valuestring[0] == '\0')
            {
                LOG_WARN("其它配置使用了自定义标记值, 但配置 %" PRIu8 " 未填写, 将跳过该配置", i + 1);
                continue;
            }
            if (mark->valuestring[0] != '\0')
            {
                login->mark = strtoul(mark->valuestring, NULL, 16);
                login->use_cus_mark = true;
                use_cus_mark = true;
                LOG_DEBUG("使用自定义标记值: %" PRIu32 " (0x%x)", login->mark, login->mark);
                LOG_DEBUG("当前使用下标: %" PRIu8, valid_i);
            }
            else
            {
                login->mark = 0x100 + valid_i * 0x100;
                LOG_DEBUG("使用自动标记值: %" PRIu32 " (0x%x)", login->mark, login->mark);
                LOG_DEBUG("当前使用下标: %" PRIu8, valid_i);
            }
        }

        // login->auto_start = auto_start->valueint;

        login->idx = i + 1;
        LOG_INFO("配置 %" PRIu8 " 可用, 将会尝试使用", i + 1);
        valid_cnt++;
        valid_i++;
    }

#else

    if (g_prog_account != 0)
    {
        LOG_INFO("非 OpenWRT 环境, 将尝试加载配置 %" PRIu8, g_prog_account);
    }
    else
    {
        LOG_INFO("非 OpenWRT 环境, 仅会尝试加载第一个有效配置");
    }

    for (uint8_t i = 0; i < cnt; i++)
    {
        /**
         * 指定了 --account 时只加载对应序号的配置,
         * 未指定时保持原有行为 (取第一个有效配置)
         */
        if (g_prog_account != 0 && i + 1 != g_prog_account)
        {
            continue;
        }

        const cJSON* account = cJSON_GetArrayItem(accounts, i);

        /**
         * 桌面端只留一个槽位: fill_account 会先把用户写过的东西按原样填进 0 号,
         * 这样配置不可用时网页上的配置页仍能回显, 不会被一次保存覆盖成空账号
         */
        login_cfg_t* login = &g_prog_status[0].login_cfg;
        const account_state_t state = fill_account(account, i + 1, login);

        if (state == ACCOUNT_INVALID)
        {
            login->time_window_count = 0;
            login->has_time_control = false;
            usable = false;
            continue;
        }
        if (state == ACCOUNT_SKIP)
        {
            usable = false;
            continue;
        }

        LOG_INFO("配置 %" PRIu8 " 可用, 将会尝试使用", i + 1);
        valid_cnt++;
        break;
    }

#endif

    cJSON_Delete(cfg_json);

    /**
     * 指定了 --account 时只保留该序号的配置
     *
     * 挑选必须放在全部配置加载完成之后:
     * 自动标记值是按"可用配置"的顺序算出来的 (0x100 + valid_i * 0x100),
     * 若在遍历时就跳过其它配置, valid_i 会从头计数, 标记值就会全部错位
     */
    if (g_prog_account != 0)
    {
        int8_t pick = -1;
        for (uint8_t i = 0; i < valid_cnt; i++)
        {
            if (g_prog_status[i].login_cfg.idx == g_prog_account)
            {
                pick = (int8_t)i;
                break;
            }
        }

        if (pick < 0)
        {
            LOG_FATAL("配置 %" PRIu8 " 不存在或该配置不可用, 请检查配置文件", g_prog_account);
            return false;
        }

        if (pick != 0)
        {
            g_prog_status[0] = g_prog_status[pick];
        }

        valid_cnt = 1;

        // 单账号进程用不到其余配置的空间, 按实际情况回收
        prog_status_t* shrunk = realloc(g_prog_status, sizeof(prog_status_t));
        if (shrunk != NULL)
        {
            g_prog_status = shrunk;
        }

        LOG_INFO("仅加载配置 %" PRIu8 ", 标记值: 0x%x", g_prog_account, g_prog_status[0].login_cfg.mark);
    }

    if (valid_cnt == 0)
    {
        LOG_FATAL("无可用配置, 请检查后重启程序");
        usable = false;
    }

    g_prog_cnt = valid_cnt;

    /**
     * 配置不可用: 返回 false, 但内存里能读出来的值已经填好了。
     * 调用方 (单进程 / Web 进程 / 监管进程) 会带着 Web 服务停下来等用户改配置,
     * 认证进程则直接退出, 交给监管者按退避策略处理
     */
    if (usable == false) return false;

    g_cfg_loaded = true;

    return true;
}
