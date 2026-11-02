#ifndef ESURFINGCLIENT_ERRORCODE_H
#define ESURFINGCLIENT_ERRORCODE_H

#include <stdbool.h>
#include <stddef.h>

#include "net/Definition.h"

/**
 * 认证失败的分类: 决定认证线程失败之后怎么等
 *
 * 分类依据来自服务端返回的两个响应头
 *   Error-Code: portal 侧的码 (1xx 网络类 / 14xxxx 认证类 / 4xx 账号类)
 *   Sub-Error:  RADIUS/ACS 侧的码 (130xxxxx / 1101xxxx)
 */
typedef enum
{
    AUTH_ERROR_NONE = 0,      // 没有拿到错误码 (或码不认识)
    AUTH_ERROR_TRANSIENT = 1, // 网络/服务端临时故障: 1/3/5/10/20/30 分钟退避, 无限重试
    AUTH_ERROR_SLOW = 2,      // 能自愈但要等: 欠费/余额/体验到期/频繁登录/别处登录/被踢, 每 30 分钟重试
    AUTH_ERROR_FATAL = 3      // 必须人工改配置: 密码错/账号不存在/版本停用/共享限制, 停到重启或退出
} auth_error_kind_t;

/**
 * @brief 按服务端返回的 Error-Code / Sub-Error 查分类与提示语
 *
 * 两个码同时出现时以 Sub-Error 为准: 它更具体, 例如 "102 + 13012000" 的真正原因是密码错
 *
 * @param error_code 响应头 Error-Code, 可为 NULL 或空串
 * @param sub_error  响应头 Sub-Error, 可为 NULL 或空串
 * @param message    命中时写入中文提示语, 可为 NULL
 * @param message_len message 的容量
 * @return 分类; 两个码都不认识时返回 AUTH_ERROR_NONE
 */
auth_error_kind_t error_code_lookup(const char* error_code, const char* sub_error, char* message, size_t message_len);

/**
 * @brief 分类名 (写日志用)
 */
const char* auth_error_kind_name(auth_error_kind_t kind);

/**
 * @brief 把一次响应里的错误码记到当前线程的认证状态上
 *
 * 没有错误码 (resp 为 NULL) 时清空, 免得把上一次失败的原因一直挂在界面上
 */
void error_code_record(const curl_resp_t* resp);

#endif
