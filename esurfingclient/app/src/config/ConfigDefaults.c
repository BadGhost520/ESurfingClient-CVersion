#include "utils/PlatformUtils.h"

#include "states/States.h"

#include "config/ConfigInternal.h"

#include "utils/Logger.h"

#include "cJSON/cJSON.h"

#include <stdio.h>

typedef enum
{
    CFG_DEF_BOOL = 0,
    CFG_DEF_NUM,
    CFG_DEF_STR,
    CFG_DEF_ARR
} cfg_def_type_t;

typedef struct
{
    const char* name;
    cfg_def_type_t type;
    bool boolean;
    int number;
    const char* str;
    const char* unit;
} cfg_def_t;

static const cfg_def_t s_cfg_defaults[] = {
    {"enabled",          CFG_DEF_BOOL, false, 0, NULL, NULL},
    {"web_external_acc", CFG_DEF_BOOL, DEFAULT_WEB_EXTERNAL_ACC, 0, NULL, NULL},
    {"log_lv",           CFG_DEF_NUM, false, LOG_LEVEL_INFO, NULL, NULL},
    {"log_dir",          CFG_DEF_STR, false, 0, DEFAULT_LOG_DIR, NULL},
    {"conn_timeout",     CFG_DEF_NUM, false, DEFAULT_CONN_TIMEOUT, NULL, "秒"},
    {"op_timeout",       CFG_DEF_NUM, false, DEFAULT_OP_TIMEOUT, NULL, "秒"},
    {"web_port",         CFG_DEF_NUM, false, DEFAULT_WEB_PORT, NULL, NULL},
};

static const cfg_def_t s_account_defaults[] = {
    {"username",     CFG_DEF_STR, false, 0, "", NULL},
    {"password",     CFG_DEF_STR, false, 0, "", NULL},
    {"channel",      CFG_DEF_NUM, false, DEFAULT_CHANNEL, NULL, NULL},
    {"mark",         CFG_DEF_STR, false, 0, "", NULL},
    {"time_windows", CFG_DEF_ARR, false, 0, NULL, NULL},
};

/**
 * @brief 把默认值渲染成日志里显示的文本
 * @param def 默认参数
 * @param out 输出缓冲
 * @param len 缓冲长度
 */
static void cfg_def_value_str(const cfg_def_t* def, char* out, const size_t len)
{
    switch (def->type)
    {
    case CFG_DEF_BOOL:
        snprintf(out, len, "%s", def->boolean ? "true" : "false");
        break;
    case CFG_DEF_NUM:
        if (def->unit != NULL) snprintf(out, len, "%d %s", def->number, def->unit);
        else snprintf(out, len, "%d", def->number);
        break;
    case CFG_DEF_STR:
        snprintf(out, len, "\"%s\"", safe_str(def->str));
        break;
    default:
        snprintf(out, len, "[]");
        break;
    }
}

/**
 * @brief 把缺的参数补进一个配置对象里
 *
 * 只补【缺的键】: 已经写过的 (哪怕写的是空值或者不合法的值) 一律不动 ——
 * 那是用户自己的选择, 该报错就报错, 不该被程序悄悄改成"看起来正常"的样子
 * @param obj 配置对象 (原地补全)
 * @param defs 默认值表
 * @param def_cnt 默认值数量
 * @param who 日志前缀 (如 "配置里" / "配置 2 ")
 * @return 补上的参数个数
 */
static uint8_t complete_cfg_obj(cJSON* obj, const cfg_def_t* defs, const size_t def_cnt, const char* who)
{
    uint8_t added = 0;

    for (size_t i = 0; i < def_cnt; i++)
    {
        const cfg_def_t* def = &defs[i];

        if (cJSON_GetObjectItem(obj, def->name) != NULL) continue;

        bool ok = false;
        switch (def->type)
        {
        case CFG_DEF_BOOL: ok = cJSON_AddBoolToObject(obj, def->name, def->boolean) != NULL; break;
        case CFG_DEF_NUM:  ok = cJSON_AddNumberToObject(obj, def->name, def->number) != NULL; break;
        case CFG_DEF_STR:  ok = cJSON_AddStringToObject(obj, def->name, safe_str(def->str)) != NULL; break;
        default:           ok = cJSON_AddArrayToObject(obj, def->name) != NULL; break;
        }

        // 补不上 (内存不够之类) 就按老路子走, 后面会打"参数不存在"的日志
        if (ok == false) continue;

        char value[TIME_WINDOW_STR_LEN];
        cfg_def_value_str(def, value, sizeof(value));
        LOG_INFO("%s缺少 %s 参数, 已按默认值 %s 补上", who, def->name, value);
        added++;
    }

    return added;
}

/**
 * @brief 补全配置文件里缺失的参数
 *
 * 手写的配置常常漏参数, 尤其是新版本加上去的 —— 不翻更新日志根本不知道有这回事。
 * 这里在读取配置的时候把缺的按默认值补上, 用户下次打开配置文件就能看见自己漏了什么。
 *
 * 补完由调用方写回文件; 写不回去也不影响本次运行 (值已经在内存里了)
 * @param cfg_json 配置对象 (原地补全)
 * @return 补上的参数个数
 */
uint8_t complete_cfg(cJSON* cfg_json)
{
    uint8_t added = complete_cfg_obj(cfg_json, s_cfg_defaults,
        sizeof(s_cfg_defaults) / sizeof(s_cfg_defaults[0]), "配置里");

    const cJSON* accounts = cJSON_GetObjectItem(cfg_json, "accounts");
    if (accounts != NULL && cJSON_IsArray(accounts))
    {
        const int cnt = cJSON_GetArraySize(accounts);
        for (int i = 0; i < cnt; i++)
        {
            cJSON* account = cJSON_GetArrayItem(accounts, i);
            if (account == NULL || cJSON_IsObject(account) == false) continue;

            char who[32];
            snprintf(who, sizeof(who), "配置 %d ", i + 1);
            added += complete_cfg_obj(account, s_account_defaults,
                sizeof(s_account_defaults) / sizeof(s_account_defaults[0]), who);
        }
    }
    else if (accounts == NULL)
    {
        /**
         * accounts 整个缺了: 补一份账号模板, 用户把账号密码填上就能跑
         *
         * 模板里的字段不再逐个打"补上了", 一次刷五行日志没人爱看
         */
        cJSON* new_accounts = cJSON_AddArrayToObject(cfg_json, "accounts");
        cJSON* account = cJSON_CreateObject();
        if (new_accounts != NULL && account != NULL)
        {
            cJSON_AddStringToObject(account, "username", "");
            cJSON_AddStringToObject(account, "password", "");
            cJSON_AddNumberToObject(account, "channel", DEFAULT_CHANNEL);
            cJSON_AddStringToObject(account, "mark", "");
            cJSON_AddArrayToObject(account, "time_windows");
            cJSON_AddItemToArray(new_accounts, account);

            LOG_INFO("配置里缺少 accounts 参数, 已补上一份账号模板 (把 username 与 password 填上)");
            added++;
        }
        else if (account != NULL)
        {
            cJSON_Delete(account);
        }
    }

    return added;
}
