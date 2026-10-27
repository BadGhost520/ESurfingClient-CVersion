#include "config/Config.h"

#include "ConfigInternal.h"

#include "core/States.h"
#include "net/NetClient.h"
#include "utils/log/Logger.h"
#include "utils/platform/PlatformUtils.h"
#include "utils/text/TextUtils.h"
#include "utils/time/TimeUtils.h"

#include <cJSON/cJSON.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

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
                                    "   \"conn_timeout\": 10,\n"
                                    "   \"op_timeout\": 15,\n"
                                    "   \"web_port\": 8888,\n"
                                    "   \"probe_hosts\": [\n"
                                    "       {\n"
                                    "           \"host\": \"connectivitycheck.platform.hicloud.com\",\n"
                                    "           \"ips\": [\"183.61.185.93\", \"125.88.255.53\"]\n"
                                    "       },\n"
                                    "       {\n"
                                    "           \"host\": \"connect.rom.miui.com\",\n"
                                    "           \"ips\": [\"202.69.4.32\", \"220.181.104.183\"]\n"
                                    "       }\n"
                                    "   ],\n"
                                    "   \"accounts\": [\n"
                                    "       {\n"
                                    "           \"username\": \"\",\n"
                                    "           \"password\": \"\",\n"
                                    "           \"channel\": 1,\n"
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

    /**
     * 先写临时文件再 rename: 现在每次启动都会写一次配置, 桌面端三个进程几乎
     * 同时启动, 直接覆盖同一个文件的话有可能互相截断 (写出半截 JSON)。
     * 临时文件名带上进程号: 多进程各写各的 tmp, 再原子替换, 谁最后写完谁生效,
     * 内容一样, 不会写坏也不会互相抢同一个 tmp。
     */
    char tmp_path[PATH_MAX + 32];
#ifdef _WIN32
    snprintf(tmp_path, sizeof(tmp_path), "%s.%lu.tmp", config_file, (unsigned long)GetCurrentProcessId());
#else
    snprintf(tmp_path, sizeof(tmp_path), "%s.%ld.tmp", config_file, (long)getpid());
#endif

    FILE* cfg_file = fopen(tmp_path, "w");
    if (!cfg_file)
    {
        LOG_ERROR("无法生成文件: %s", tmp_path);
        free(cfg_text);
        return false;
    }
    const bool written = (fprintf(cfg_file, "%s", cfg_text) > 0);
    fflush(cfg_file);
    fclose(cfg_file);
    free(cfg_text);

    if (written == false)
    {
        remove(tmp_path);
        return false;
    }

#ifdef _WIN32
    const bool replaced = (MoveFileExA(tmp_path, config_file, MOVEFILE_REPLACE_EXISTING) != 0);
#else
    const bool replaced = (rename(tmp_path, config_file) == 0);
#endif

    if (replaced == false)
    {
        LOG_ERROR("替换配置文件失败: %s", config_file);
        remove(tmp_path);
    }
    return replaced;
}

/**
 * @brief 把内存里的配置按固定键序写成 JSON 文件
 *
 * 和"在旧 JSON 上补键"不同: 这里从 cfg 结构重新生成, 补全的参数会出现在默认
 * 模板的位置, 而不是被追加到文件末尾。
 *
 * 账号一律从 cfg_json 原样带出来 (complete_cfg 已经补全缺失的键), 不能从
 * g_prog_status 重建 —— OpenWrt 上每个进程只加载自己那一个账号, 按它写回会
 * 把配置文件里别的账号直接抹掉。
 *
 * @param cfg_json 已补全的配置对象 (账号仍然完整)
 * @return 是否写入成功
 */
static bool save_cfg_from_cfg(const cJSON* cfg_json)
{
    const cfg_t* global = &g_prog_status[0].cfg;

    cJSON* root = cJSON_CreateObject();
    if (root == NULL) return false;

    cJSON_AddBoolToObject(root, "enabled", global->enabled);
    cJSON_AddBoolToObject(root, "web_external_acc", global->web_external_acc);
    cJSON_AddNumberToObject(root, "log_lv", global->log_lv);
    cJSON_AddStringToObject(root, "log_dir", global->log_dir);
    cJSON_AddNumberToObject(root, "conn_timeout", global->conn_timeout);
    cJSON_AddNumberToObject(root, "op_timeout", global->op_timeout);
    cJSON_AddNumberToObject(root, "web_port", global->web_port);

    cJSON* probes = cJSON_AddArrayToObject(root, "probe_hosts");
    for (uint8_t i = 0; probes != NULL && i < global->probe_host_count; i++)
    {
        cJSON* item = cJSON_CreateObject();
        if (item == NULL) break;

        cJSON_AddStringToObject(item, "host", global->probe_hosts[i].host);
        cJSON* ips = cJSON_AddArrayToObject(item, "ips");
        for (uint8_t k = 0; ips != NULL && k < global->probe_hosts[i].ip_count; k++)
        {
            cJSON_AddItemToArray(ips, cJSON_CreateString(global->probe_hosts[i].ips[k]));
        }
        cJSON_AddItemToArray(probes, item);
    }

    cJSON* accounts = cJSON_AddArrayToObject(root, "accounts");
    if (accounts != NULL)
    {
        const cJSON* src = cJSON_GetObjectItem(cfg_json, "accounts");
        if (cJSON_IsArray(src))
        {
            const int count = cJSON_GetArraySize(src);
            for (int i = 0; i < count; i++)
            {
                cJSON* copy = cJSON_Duplicate(cJSON_GetArrayItem(src, i), 1);
                if (copy != NULL) cJSON_AddItemToArray(accounts, copy);
            }
        }
    }

    /**
     * 一个账号都没有时 (配置被禁用/账号没填) 也要写一个空账号模板:
     * 否则写回去的就是 accounts: [], 和新建配置时的样子不一致, 用户下次
     * 打开文件还得自己回想字段名
     */
    if (accounts != NULL && cJSON_GetArraySize(accounts) == 0)
    {
        cJSON* item = cJSON_CreateObject();
        if (item != NULL)
        {
            cJSON_AddStringToObject(item, "username", "");
            cJSON_AddStringToObject(item, "password", "");
            cJSON_AddNumberToObject(item, "channel", DEFAULT_CHANNEL);
            cJSON_AddStringToObject(item, "mark", "");
            cJSON_AddArrayToObject(item, "time_windows");
            cJSON_AddItemToArray(accounts, item);
        }
    }

    const bool ok = write_cfg_json(root);
    cJSON_Delete(root);
    return ok;
}

#ifndef __OPENWRT__

bool save_cfg(const char* configs_str)
{
    LOG_DEBUG("保存配置中");
    LOG_DEBUG("仅会保存第一个可用配置");

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

const char* get_config_path(void)
{
    return config_file;
}

static bool s_cfg_waiting = false;

#define CFG_WAIT_TICK_MS 1000

/** @brief "保存并应用"标记文件: 没有控制通道的监管者/Web 进程靠它知道该重新读配置 */
#define CFG_APPLY_MARK_SUFFIX ".apply"

static void cfg_apply_mark_path(char* out, const size_t len)
{
    snprintf(out, len, "%s%s", get_config_path(), CFG_APPLY_MARK_SUFFIX);
}

void cfg_request_apply(void)
{
    char path[PATH_MAX + 16];
    cfg_apply_mark_path(path, sizeof(path));

    FILE* f = fopen(path, "w");
    if (f != NULL) fclose(f);
}

bool cfg_take_apply_request(void)
{
    char path[PATH_MAX + 16];
    cfg_apply_mark_path(path, sizeof(path));

    FILE* f = fopen(path, "r");
    if (f == NULL) return false;

    fclose(f);
    remove(path);
    return true;
}

/**
 * @brief 读取配置文件的原文
 *
 * 程序还没把配置读起来时 (停在"等配置"状态, 或者网页刚保存、重新加载还没跑完),
 * 网页直接展示文件里的内容: 内存里那份是空的或者旧的, 显示成空账号的话,
 * 用户一保存就把自己写过的账号密码覆盖掉了
 * @return 文件内容 (需要 free), 读不到或文件为空时返回 NULL
 */
char* read_cfg_file(void)
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
 * @brief 等配置文件变成可用 (每次最多等一个固定的间隔)
 *
 * 只有网页上的"保存并应用"会置 g_need_reload 才重新解析: 单纯保存只写文件,
 * 这里不跟文件内容走 —— 否则用户一保存配置就被悄悄加载, 和"重启才生效"的约定冲突。
 * @return true 表示配置已可用 (此时 load_cfg 已经跑完)
 */
bool wait_cfg_usable(void)
{
    if (s_cfg_waiting == false)
    {
        s_cfg_waiting = true;
        g_need_reload = false;
    }

    sleep_ms(CFG_WAIT_TICK_MS, false);

    // 只有"应用"才重新加载
    if (g_need_reload == false) return false;

    g_need_reload = false;

    if (load_cfg() == true)
    {
        s_cfg_waiting = false;
        LOG_DEBUG("配置已可用, 继续往下走");
        return true;
    }

    LOG_DEBUG("配置仍不可用, 请在网页界面上改好后再点「保存并应用」");
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
static account_state_t fill_account(const cJSON* account, const uint8_t cfg_no, cfg_t* cfg)
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

bool load_cfg(void)
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
        LOG_DEBUG("创建新的默认配置文件");
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
     * 放在这里是为了后面每一处解析都能拿到完整的对象。补全的结果最终从 cfg 结构
     * 重新生成并写回文件 (见文件末尾), 所以补出来的键会落在默认模板的位置。
     */
    const uint8_t completed = complete_cfg(cfg_json);
    if (completed > 0)
    {
        LOG_INFO("配置文件缺少参数, 已按默认值补全 %" PRIu8 " 个", completed);
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

    const long log_lv = cfg_read_num(cfg_json, "log_lv", LOG_LEVEL_INFO);
    set_logger_level((log_level_t)log_lv);

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
        LOG_DEBUG("OpenWRT 环境, 本次仅使用配置 %" PRIu8, g_prog_account);
    }
    else
    {
        LOG_DEBUG("OpenWRT 环境, 会尝试加载所有有效配置");
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

        cfg_t* login = &g_prog_status[valid_i].cfg;
        const account_state_t state = fill_account(account, i + 1, login);
        if (state == ACCOUNT_SKIP) continue;
        if (state == ACCOUNT_INVALID)
        {
            cJSON_Delete(cfg_json);
            return false;
        }

        /**
         * mark 必须是字符串 (如 "0x100")。
         *
         * 写成数字 ("mark": 256) 或对象时 cJSON 的 valuestring 是 NULL, 下面直接取
         * [0] 就是段错误 —— 配置写错一个字段就崩进程。这类写法一律按"没填"处理,
         * 让它走自动分配 (或按下面的自定义 mark 规则跳过)。
         */
        if (mark != NULL && cJSON_IsString(mark) == false)
        {
            LOG_WARN("配置 %" PRIu8 " 填写了非法 mark 值, 将忽略该 mark 值", i + 1);
            mark = NULL;
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
        LOG_DEBUG("配置 %" PRIu8 " 可用, 将会尝试使用", i + 1);
        valid_cnt++;
        valid_i++;
    }

#else

    if (g_prog_account != 0)
    {
        LOG_DEBUG("非 OpenWRT 环境, 将尝试加载配置 %" PRIu8, g_prog_account);
    }
    else
    {
        LOG_DEBUG("非 OpenWRT 环境, 仅会尝试加载第一个有效配置");
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
        cfg_t* login = &g_prog_status[0].cfg;
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

        LOG_DEBUG("配置 %" PRIu8 " 可用, 将会尝试使用", i + 1);
        valid_cnt++;
        break;
    }

#endif

    /**
     * 指定了 --account 时只保留该序号的配置
     *
     * 挑选必须放在全部配置加载完成之后:
     * 自动标记值是按"可用配置"的顺序算出来的 (0x100 + valid_i * 0x100),
     * 若在遍历时就跳过其它配置, valid_i 会从头计数, 标记值就会全部错位
     *
     * cfg_json 要留到最后写回配置时再用, 这里先不能删
     */
    if (g_prog_account != 0)
    {
        int8_t pick = -1;
        for (uint8_t i = 0; i < valid_cnt; i++)
        {
            if (g_prog_status[i].cfg.idx == g_prog_account)
            {
                pick = (int8_t)i;
                break;
            }
        }

        if (pick < 0)
        {
            LOG_FATAL("配置 %" PRIu8 " 不存在或该配置不可用, 请检查配置文件", g_prog_account);
            cJSON_Delete(cfg_json);
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

        LOG_DEBUG("仅加载配置 %" PRIu8 ", 标记值: 0x%x", g_prog_account, g_prog_status[0].cfg.mark);
    }

    if (valid_cnt == 0)
    {
        LOG_FATAL("无可用配置, 请检查后重启程序");
        usable = false;
    }

    g_prog_cnt = valid_cnt;

    /**
     * 全局配置与探测目标必须在【所有分支和 --account 挑选都结束之后】写:
     * - OpenWrt 分支上面会 realloc 掉 g_prog_status, 提前写会写进旧块, memset 后全变 0
     * - --account 挑选会把 g_prog_status[pick] 整体拷到 [0], 提前写会被那次拷贝覆盖
     *
     * 放在 cJSON_Delete(cfg_json) 之前, 因为探测目标还要从 cfg_json 里取
     */
    {
        cfg_t* global_cfg = &g_prog_status[0].cfg;

        global_cfg->enabled = g_prog_enabled;
        global_cfg->web_external_acc = g_web_external_acc;
        global_cfg->log_lv = log_lv;
        snprintf(global_cfg->log_dir, sizeof(global_cfg->log_dir), "%s",
            log_dir != NULL ? log_dir : get_logger_dir_cfg());
        global_cfg->web_port = g_web_port;
        global_cfg->conn_timeout = g_conn_timeout;
        global_cfg->op_timeout = g_op_timeout;

        /**
         * 联网探测目标 (probe_hosts)
         *
         * 缺这一项时 complete_cfg 已经按默认值补上; 有就完全按用户的来 (写几组用几组,
         * 不擅自补)。格式不对则回退内置默认。
         */
        probe_targets_defaults();
        if (probe_targets_parse(cJSON_GetObjectItem(cfg_json, "probe_hosts")) == false)
        {
            LOG_WARN("probe_hosts 格式不对, 使用内置默认探测目标");
        }
    }

    /**
     * 每次加载都把配置写回一次: 全局字段与探测目标用当前进程解析出来的值,
     * 账号原样带出来。没缺失的时候也等于顺手把用户手写的格式统一成程序格式。
     * 写不回去不影响本次运行, 值已经在内存里
     */
    if (save_cfg_from_cfg(cfg_json))
    {
        LOG_DEBUG("配置已写回 %s", config_file);
    }
    else
    {
        LOG_WARN("配置写回失败 (配置文件可能是只读的?), 本次仍按内存里的值运行");
    }

    // 写回用完了, 这里再释放 (上面按 --account 挑选、写回都还要读它)
    cJSON_Delete(cfg_json);

    /**
     * 配置不可用: 返回 false, 但内存里能读出来的值已经填好了。
     * 调用方 (认证进程 / Web 进程 / 监管进程) 会带着 Web 服务停下来等用户改配置,
     * 认证进程则直接退出, 交给监管者按退避策略处理
     */
    if (usable == false) return false;

    g_cfg_loaded = true;

    return true;
}
