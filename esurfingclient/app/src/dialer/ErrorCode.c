#include "dialer/ErrorCode.h"

#include <stdio.h>
#include <string.h>

#include "core/States.h"
#include "utils/log/Logger.h"
#include "utils/time/TimeUtils.h"

/**
 * 错误码表 (只收服务端可能返回的那批)
 *
 * 数据来源: 广东天翼校园客户端故障指引 (客户端 ErrorView 里的 GDWinErrContent.js)。
 * Windows 客户端本地自检的码 (3xx 服务类 / 500 组件 / 20010101~05 共享冲突检测 /
 * 1700xxxx 上报 / 991~993 网络自检) 与我们无关, 没有收录。
 *
 * code 同时匹配 Error-Code 与 Sub-Error: 两边的码空间不重叠
 */
typedef struct
{
    const char* code;
    auth_error_kind_t kind;
    const char* message;
} error_entry_t;

static const error_entry_t s_errors[] = {
    // ==== Sub-Error: RADIUS / ACS 侧 (130xxxxx / 1101xxxx) ====
    {"11010000", AUTH_ERROR_TRANSIENT, "找不到 SDX 信息, 请确认校园网端口是否已开通"},
    {"11064000", AUTH_ERROR_SLOW, "登录过于频繁, 请过一段时间再试"},
    {"13001000", AUTH_ERROR_FATAL, "认证请求被拒绝, 请确认账号密码是否正确、账号是否欠费"},
    {"13003000", AUTH_ERROR_TRANSIENT, "认证服务器忙, 请稍后再试"},
    {"13004000", AUTH_ERROR_TRANSIENT, "登录失败, 可能是账号密码错误、欠费或账号状态异常"},
    {"13005000", AUTH_ERROR_TRANSIENT, "请求认证超时, 请检查网络后重试"},
    {"13012000", AUTH_ERROR_FATAL, "帐号或密码错误"},
    {"13014000", AUTH_ERROR_SLOW, "终端数超出限制, 请先退出其他已登录终端"},
    {"13015000", AUTH_ERROR_SLOW, "账户余额不足, 请充值后重试"},
    {"13016000", AUTH_ERROR_FATAL, "输入的帐号不存在, 帐号无需加 @ 后缀"},
    {"13017000", AUTH_ERROR_SLOW, "账号欠费, 请充值后重试"},
    {"13018000", AUTH_ERROR_FATAL, "拨号类型错误, 该账号不支持 portal 认证"},
    {"13019000", AUTH_ERROR_FATAL, "终端类型错误, 该账号不支持当前终端类型拨号"},
    {"13020000", AUTH_ERROR_SLOW, "体验活动已结束"},
    {"13021000", AUTH_ERROR_SLOW, "体验帐号已过期"},
    {"13022000", AUTH_ERROR_SLOW, "今日体验时长已用完, 请明天再试"},

    // ==== Error-Code: portal 认证服务 ====
    {"140001", AUTH_ERROR_TRANSIENT, "服务端认为这是无效的请求"},
    {"140002", AUTH_ERROR_FATAL, "客户端版本不正确, 请更新客户端"},
    {"140011", AUTH_ERROR_FATAL, "服务端拿到的客户端 MAC 地址无效"},
    {"140013", AUTH_ERROR_FATAL, "算法 ID 已失效, 请更新客户端"},
    {"140014", AUTH_ERROR_TRANSIENT, "登录校验失败, 请稍后重试"},
    {"140015", AUTH_ERROR_FATAL, "校验值错误, 请检查账号配置"},
    {"140100", AUTH_ERROR_FATAL, "账号或密码错误, 请重新输入"},
    {"140103", AUTH_ERROR_TRANSIENT, "网关可容纳的接入数已满, 请稍后重试"},
    {"140104", AUTH_ERROR_FATAL, "对应的用户不存在"},
    {"140105", AUTH_ERROR_FATAL, "用户账号受限, 请联系校园网络维护人员"},
    {"140200", AUTH_ERROR_TRANSIENT, "密钥过期, 请重新认证"},
    {"140300", AUTH_ERROR_TRANSIENT, "正在等待终端发起认证"},
    {"140301", AUTH_ERROR_TRANSIENT, "正在等待服务端进行验证"},

    // ==== Error-Code: 网络与连接 ====
    {"101", AUTH_ERROR_TRANSIENT, "网络不通, 请检查本机 IP、网关与 DNS"},
    {"102", AUTH_ERROR_TRANSIENT, "网络异常, 登录信息获取失败"},
    {"103", AUTH_ERROR_TRANSIENT, "网络异常, 拨号请求发送失败"},
    {"104", AUTH_ERROR_TRANSIENT, "网络异常, 获取 ticket 失败"},
    {"105", AUTH_ERROR_TRANSIENT, "网络异常, 网络状态检查失败"},
    {"106", AUTH_ERROR_TRANSIENT, "网络异常, 下线请求发送失败"},
    {"107", AUTH_ERROR_TRANSIENT, "网络异常, 请检查网线是否被拔、无线是否已断开"},
    {"108", AUTH_ERROR_TRANSIENT, "网络异常, 请检查是否已接入校园网"},
    {"109", AUTH_ERROR_TRANSIENT, "连接客户端服务器失败"},
    {"140000", AUTH_ERROR_NONE, "服务端没有记录错误"},
    {"301", AUTH_ERROR_TRANSIENT, "连接数据库服务器失败"},
    {"302", AUTH_ERROR_TRANSIENT, "连接前置服务器失败"},
    {"303", AUTH_ERROR_TRANSIENT, "连接客户端服务器失败"},

    // ==== Error-Code: 账号状态 ====
    {"401", AUTH_ERROR_SLOW, "该帐号已在别处登录, 若不是本人操作请及时改密码"},
    {"403", AUTH_ERROR_FATAL, "客户端版本已被停用, 请更新客户端"},
    {"315", AUTH_ERROR_SLOW, "已被其他终端踢下线"},
    {"20010005", AUTH_ERROR_FATAL, "配置文件被破坏, 请重新填写账号信息"},
    {"20010009", AUTH_ERROR_SLOW, "网络已断开, 或账号已在别处登录"},
    {"-600", AUTH_ERROR_FATAL, "检测到网络共享行为, 账号被限制登录, 请关闭共享"},
};

#define ERROR_COUNT (sizeof(s_errors) / sizeof(s_errors[0]))

/**
 * @brief 在表里精确找一码
 */
static const error_entry_t* error_code_find(const char* code)
{
    if (code == NULL || code[0] == '\0') return NULL;
    for (size_t i = 0; i < ERROR_COUNT; i++)
    {
        if (strcmp(s_errors[i].code, code) == 0) return &s_errors[i];
    }
    return NULL;
}

auth_error_kind_t error_code_lookup(const char* error_code, const char* sub_error, char* message, size_t message_len)
{
    // Sub-Error 更具体, 优先用它
    const error_entry_t* entry = error_code_find(sub_error);
    if (entry == NULL) entry = error_code_find(error_code);
    if (entry == NULL)
    {
        if (message != NULL && message_len > 0) message[0] = '\0';
        return AUTH_ERROR_NONE;
    }

    if (message != NULL && message_len > 0)
    {
        snprintf(message, message_len, "%s", entry->message);
    }
    return entry->kind;
}

const char* auth_error_kind_name(const auth_error_kind_t kind)
{
    switch (kind)
    {
    case AUTH_ERROR_TRANSIENT:
        return "临时错误";
    case AUTH_ERROR_SLOW:
        return "可自愈错误";
    case AUTH_ERROR_FATAL:
        return "需人工处理";
    default:
        return "未知";
    }
}

void error_code_record(const curl_resp_t* resp)
{
    if (tl_thread_idx < 0 || g_prog_status == NULL) return;

    runtime_status_t* status = &g_prog_status[tl_thread_idx].runtime_status;
    status->last_error_code[0] = '\0';
    status->last_sub_error[0] = '\0';
    status->last_error_msg[0] = '\0';
    status->last_error_kind = AUTH_ERROR_NONE;
    status->last_error_at = 0;

    if (resp == NULL) return;
    if (resp->error_code[0] == '\0' && resp->sub_error[0] == '\0') return;

    snprintf(status->last_error_code, sizeof(status->last_error_code), "%s", resp->error_code);
    snprintf(status->last_sub_error, sizeof(status->last_sub_error), "%s", resp->sub_error);
    status->last_error_kind = (uint8_t)error_code_lookup(resp->error_code, resp->sub_error,
                                                         status->last_error_msg, sizeof(status->last_error_msg));
    status->last_error_at = get_cur_tm_ms();

    LOG_DEBUG("记录服务端错误码: Error-Code=%s, Sub-Error=%s, 分类=%s",
              status->last_error_code[0] != '\0' ? status->last_error_code : "-",
              status->last_sub_error[0] != '\0' ? status->last_sub_error : "-",
              auth_error_kind_name((auth_error_kind_t)status->last_error_kind));
}
