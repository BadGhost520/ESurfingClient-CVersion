#include "WebInternal.h"

#include <mongoose/mongoose.h>

#include "config/Config.h"

#include "control/Control.h"

#include "core/States.h"

#include "utils/platform/PlatformUtils.h"
#include "utils/log/Logger.h"

#include "webserver/HotspotControl.h"

#include <cJSON/cJSON.h>

/**
 * @brief 请求重新认证
 * @return 是否成功
 */
static bool request_restart_auth(void)
{
    if (s_local_state)
    {
        if (g_prog_status == NULL || g_prog_cnt <= 0) return false;
        g_prog_status[0].runtime_status.is_need_reauth = true;
        return true;
    }

    return control_restart_auth();
}

/**
 * @brief 请求重新加载配置并重新认证
 * @return 是否成功
 */
static bool request_apply_config(void)
{
    if (s_local_state)
    {
        if (g_prog_status == NULL || g_prog_cnt <= 0) return false;
        g_cfg_loaded = false;
        g_prog_status[0].runtime_status.is_need_reauth = true;
        return true;
    }

    return control_apply_config();
}

/**
 * @brief 处理 POST API 请求
 * @return 是否已处理
 */
bool handle_api_post(struct mg_connection* c, struct mg_http_message* hm)
{
    // 从 Web 线程读取请求体
    char* data = NULL;
    if (hm->body.len > 0)
    {
        data = malloc(hm->body.len + 1);
        if (data == NULL)
        {
            mg_http_reply(c, 500, "", "");
            return true;
        }
        memcpy(data, hm->body.buf, hm->body.len);
        data[hm->body.len] = '\0';
    }

    // 仅保存
    if (mg_match(hm->uri, mg_str("/api/saveConfigs"), NULL))
    {
        if (data == NULL)
        {
            mg_http_reply(c, 400, "", "");
            return true;
        }
        if (save_cfg(data))
        {
            mg_http_reply(c, 204, "", "");
        }
        else
        {
            mg_http_reply(c, 500, "", "");
        }
        free(data);
        return true;
    }

    // 仅应用
    if (mg_match(hm->uri, mg_str("/api/applyConfigs"), NULL))
    {
        if (data == NULL)
        {
            mg_http_reply(c, 400, "", "");
            return true;
        }

        cJSON* operation_json = cJSON_Parse(data);
        if (operation_json == NULL)
        {
            mg_http_reply(c, 500, "", "");
            free(data);
            return true;
        }

        const cJSON* apply = cJSON_GetObjectItem(operation_json, "apply");
        if (apply)
        {
            if (cJSON_IsBool(apply))
            {
                /**
                 * 配置不可用时程序正停在"等配置"的状态, 网页开着就是为了让用户
                 * 把配置改回来 —— 这时"保存并应用"就是让主流程重新读一遍配置的
                 * 开关: 没有认证线程可下发, 也不需要下发
                 */
                if (g_waiting_cfg)
                {
                    g_need_reload = true;
                    LOG_INFO("收到 Web 应用配置文件请求, 主流程将重新加载配置文件");
                    mg_http_reply(c, 204, "", "");
                }
                else if (g_prog_status == NULL)
                {
                    LOG_WARN("收到 Web 应用配置文件请求, 但程序尚未加载完成, 拒绝操作");
                    mg_http_reply(c, 403, "", "");
                }
                else
                {
                    bool is_authed = false;
                    const bool reachable = query_is_authed(&is_authed);

                    /**
                     * 只要认证进程在 (reachable), 就把"重新加载配置"的请求发下去。
                     *
                     * 不要求"正在认证": 配置刚加载完时认证线程还没起 (要等网络进入
                     * "需要认证"的状态), 而这恰恰是用户会去改配置的时候 ——
                     * 卡在这里的话, 用户在等网络时把程序关掉会收到一个"应用失败",
                     * 程序却照旧拿着旧配置往下走
                     */
                    if (reachable == false)
                    {
                        LOG_WARN("收到 Web 应用配置文件请求, 但认证进程不可达");
                        mg_http_reply(c, 503, "", "");
                    }
                    else if (request_apply_config() == false)
                    {
                        LOG_WARN("收到 Web 应用配置文件请求, 但下发到认证进程失败");
                        mg_http_reply(c, 503, "", "");
                    }
                    else
                    {
                        LOG_INFO("收到 Web 应用配置文件请求 (当前%s), 程序将重新加载配置文件",
                            is_authed ? "已认证" : "未认证");
                        mg_http_reply(c, 204, "", "");
                    }
                }
            }
            else
            {
                mg_http_reply(c, 500, "", "");
            }
        }
        else
        {
            mg_http_reply(c, 500, "", "");
        }

        free(data);
        cJSON_Delete(operation_json);
        return true;
    }

    // 重新认证
    if (mg_match(hm->uri, mg_str("/api/restartAuth"), NULL))
    {
        if (g_prog_status == NULL || g_cfg_loaded == false)
        {
            LOG_WARN("收到 Web 重新认证请求, 但程序尚未加载配置, 拒绝操作");
            mg_http_reply(c, 503, "", "");
        }
        else
        {
            bool is_authed = false;
            const bool reachable = query_is_authed(&is_authed);

            if (reachable == false)
            {
                LOG_WARN("收到 Web 重新认证请求, 但认证进程不可达");
                mg_http_reply(c, 503, "", "");
            }
            else if (is_authed == false)
            {
                LOG_WARN("收到 Web 重新认证请求, 但没有线程在认证, 拒绝操作");
                mg_http_reply(c, 403, "", "");
            }
            else if (request_restart_auth() == false)
            {
                LOG_WARN("收到 Web 重新认证请求, 但下发到认证进程失败");
                mg_http_reply(c, 503, "", "");
            }
            else
            {
                LOG_INFO("收到 Web 重新认证请求, 认证线程将重新进行认证");
                mg_http_reply(c, 204, "", "");
            }
        }

        free(data);
        return true;
    }

    free(data);

    /**
     * 热点开关: 立刻回"已受理", 真正的切换在后台线程里跑 (要起一次 PowerShell,
     * 快则一秒慢则十几秒, 不能占着 Web 线程); 页面随后轮询 /api/hotspot/status 看结果
     */
    if (mg_match(hm->uri, mg_str("/api/hotspot/start"), NULL) ||
        mg_match(hm->uri, mg_str("/api/hotspot/stop"), NULL))
    {
        const bool is_start = mg_match(hm->uri, mg_str("/api/hotspot/start"), NULL);
        char adapter[ADAPTER_ID_LEN] = {0};
        const int adapter_len = mg_http_get_var(&hm->query, "adapter", adapter, sizeof(adapter));
        if (adapter_len <= 0)
        {
            LOG_WARN("收到热点%s请求, 但没有指定网卡", is_start ? "开启" : "关闭");
            mg_http_reply(c, 400, "", "");
            return true;
        }

        char* body = NULL;
        const int status = hotspot_request(is_start ? "start" : "stop", adapter, &body);
        if (body != NULL)
        {
            mg_http_reply(c, status, HEADER_JSON, "%s", body);
            free(body);
        }
        else
        {
            mg_http_reply(c, status, "", "");
        }
        return true;
    }

    // 未知的 POST 路径
    mg_http_reply(c, 404, HEADER_TEXT, "Not found\n");
    return true;
}
