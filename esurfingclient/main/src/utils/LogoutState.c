#include "utils/PlatformUtils.h"
#include "utils/LogoutState.h"
#include "utils/Logger.h"

#include "cJSON/cJSON.h"

#include <stdlib.h>
#include <stdio.h>

/**
 * @brief 拼出存档路径: <配置文件路径>.<账号序号>.logout
 *
 * 每个账号一个文件: OpenWrt 上每个账号一个进程, 共用一个文件会互相覆盖。
 * @param out 输出缓冲
 * @param len 缓冲长度
 * @param idx 账号序号
 * @return 是否成功
 */
static bool logout_state_path(char* out, const size_t len, const uint8_t idx)
{
    const char* cfg = get_config_path();
    if (cfg == NULL || cfg[0] == '\0') return false;

    const int n = snprintf(out, len, "%s.%" PRIu8 ".logout", cfg, idx);
    return n > 0 && (size_t)n < len;
}

bool logout_state_save(const prog_status_t* status)
{
    if (status == NULL) return false;

    char path[PATH_MAX + 32];
    if (logout_state_path(path, sizeof(path), status->login_cfg.idx) == false)
    {
        LOG_WARN("无法确定会话存档路径, 本次退出若被强杀将无法补登出");
        return false;
    }

    cJSON* root = cJSON_CreateObject();
    if (root == NULL) return false;

    cJSON_AddNumberToObject(root, "account", status->login_cfg.idx);
    cJSON_AddStringToObject(root, "user_agent", safe_str(status->login_cfg.user_agent));
    cJSON_AddStringToObject(root, "term_url", safe_str(status->auth_cfg.term_url));
    cJSON_AddStringToObject(root, "algo_id", safe_str(status->auth_cfg.algo_id));
    cJSON_AddStringToObject(root, "client_id", safe_str(status->auth_cfg.client_id));
    cJSON_AddStringToObject(root, "host_name", safe_str(status->auth_cfg.host_name));
    cJSON_AddStringToObject(root, "client_ip", safe_str(status->auth_cfg.client_ip));
    cJSON_AddStringToObject(root, "mac_addr", safe_str(status->auth_cfg.mac_addr));
    cJSON_AddStringToObject(root, "ostag", safe_str(status->auth_cfg.ostag));
    cJSON_AddStringToObject(root, "ticket", safe_str(status->auth_cfg.ticket));

    char* text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (text == NULL) return false;

    FILE* fp = fopen(path, "w");
    if (fp == NULL)
    {
        LOG_WARN("无法写入会话存档 %s, 本次退出若被强杀将无法补登出", path);
        free(text);
        return false;
    }

    const bool ok = fprintf(fp, "%s", text) > 0;
    fclose(fp);
    free(text);

    if (ok == false)
    {
        LOG_WARN("会话存档写入不完整: %s", path);
        return false;
    }

    LOG_DEBUG("会话现场已存档: %s (被强杀时下次启动会补登出)", path);
    return true;
}

/**
 * @brief 从 JSON 里取一个字符串字段, 按目标缓冲区长度安全截断
 */
static void read_str(const cJSON* root, const char* key, char* out, const size_t len)
{
    const cJSON* item = cJSON_GetObjectItem(root, key);
    if (item == NULL || cJSON_IsString(item) == false || item->valuestring == NULL) return;

    snprintf(out, len, "%s", item->valuestring);
}

bool logout_state_load(prog_status_t* status)
{
    if (status == NULL) return false;

    char path[PATH_MAX + 32];
    if (logout_state_path(path, sizeof(path), status->login_cfg.idx) == false) return false;

    FILE* fp = fopen(path, "r");
    if (fp == NULL) return false;

    char data[2048];
    const size_t got = fread(data, 1, sizeof(data) - 1, fp);
    fclose(fp);
    data[got] = '\0';

    cJSON* root = cJSON_Parse(data);
    if (root == NULL)
    {
        LOG_WARN("会话存档解析失败 (可能上次写到一半就被杀了), 按没有存档处理: %s", path);
        return false;
    }

    // 存档里的账号序号必须与本次负责的账号一致, 否则说明文件放错了地方
    const cJSON* account = cJSON_GetObjectItem(root, "account");
    if (account == NULL || cJSON_IsNumber(account) == false ||
        (uint8_t)account->valueint != status->login_cfg.idx)
    {
        LOG_WARN("会话存档的账号序号与本次不符, 忽略: %s", path);
        cJSON_Delete(root);
        return false;
    }

    read_str(root, "user_agent", status->login_cfg.user_agent, USER_AGENT_LEN);
    read_str(root, "term_url", status->auth_cfg.term_url, TERM_URL_LEN);
    read_str(root, "algo_id", status->auth_cfg.algo_id, ALGO_ID_LEN);
    read_str(root, "client_id", status->auth_cfg.client_id, CLIENT_ID_LEN);
    read_str(root, "host_name", status->auth_cfg.host_name, HOST_NAME_LEN);
    read_str(root, "client_ip", status->auth_cfg.client_ip, IP_LEN);
    read_str(root, "mac_addr", status->auth_cfg.mac_addr, MAC_ADDR_LEN);
    read_str(root, "ostag", status->auth_cfg.ostag, OSTAG_LEN);
    read_str(root, "ticket", status->auth_cfg.ticket, TICKET_LEN);

    cJSON_Delete(root);

    /**
     * 没有 term_url 就没法登出, 这份存档是废的 —— 直接当没有,
     * 免得调用方拿着一份空现场去发请求
     */
    if (status->auth_cfg.term_url[0] == '\0' || status->auth_cfg.algo_id[0] == '\0')
    {
        LOG_WARN("会话存档缺少 term_url 或 algo_id, 无法补登出: %s", path);
        return false;
    }

    return true;
}

void logout_state_clear(const uint8_t idx)
{
    char path[PATH_MAX + 32];
    if (logout_state_path(path, sizeof(path), idx) == false) return;

    if (remove(path) == 0)
    {
        LOG_DEBUG("会话存档已清除: %s", path);
    }
}
