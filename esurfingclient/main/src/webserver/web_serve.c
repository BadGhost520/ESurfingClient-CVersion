#include "webserver/web_internal.h"

#include <mongoose/mongoose.h>

#include "utils/PlatformUtils.h"
#include "utils/Logger.h"

int str_case_cmp(const char* a, const char* b)
{
#ifdef _WIN32
    return _stricmp(a, b);
#else
    return strcasecmp(a, b);
#endif
}

#ifdef _WIN32
/**
 * @brief 通过文件句柄获取文件大小与修改时间
 * @note FindFirstFile 读取的是目录项, 正在被写入的日志文件 (run.log) 的大小与时间
 *       可能还没有同步到目录项, 这里用句柄再取一次真实值
 * @return 是否获取成功
 */
bool win_stat_file(const char* path, uint64_t* size, uint64_t* mtime)
{
    HANDLE handle = CreateFileA(path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER file_size;
    BY_HANDLE_FILE_INFORMATION info;
    const bool ok = GetFileSizeEx(handle, &file_size) != 0 &&
        GetFileInformationByHandle(handle, &info) != 0;
    CloseHandle(handle);
    if (ok == false) return false;

    if (size != NULL) *size = (uint64_t)file_size.QuadPart;
    if (mtime != NULL)
    {
        const uint64_t file_time = ((uint64_t)info.ftLastWriteTime.dwHighDateTime << 32) |
            (uint64_t)info.ftLastWriteTime.dwLowDateTime;
        *mtime = file_time / 10000000ULL - 11644473600ULL;
    }
    return true;
}
#endif

/**
 * @brief 网页文件所在目录
 *
 * 必须按【程序所在目录】拼绝对路径, 不能写成相对路径 "portal":
 * 那样它会相对于进程的当前工作目录, 而配置文件是按程序目录找的, 两者不一致。
 * 把程序放进 /usr/local/bin 再从别处启动 (教程里就这么建议的) 时,
 * 网页文件会找不到, 打开页面只有 404。
 * @return 目录路径
 */
static const char* web_root_dir(void)
{
    static char dir[PATH_MAX] = "";

    if (dir[0] != '\0') return dir;

    char exec_dir[PATH_MAX];
    if (get_exec_dir(exec_dir) == false)
    {
        LOG_WARN("无法获取程序所在目录, 网页文件将按当前目录下的 portal 查找");
        return "portal";
    }

    snprintf(dir, sizeof(dir), "%s%cportal", safe_str(exec_dir), SEP);
    return dir;
}

void fn(struct mg_connection *c, const int ev, void *ev_data)
{
    if (ev != MG_EV_HTTP_MSG) return;

    struct mg_http_message* hm = ev_data;

    // GET 请求
    if (mg_strcmp(hm->method, mg_str("GET")) == 0)
    {
        // 命中 API 时直接返回, 避免静态文件处理重复发送响应
        if (handle_api_get(c, hm)) return;

            struct mg_http_serve_opts opts = { .root_dir = web_root_dir() };
        mg_http_serve_dir(c, hm, &opts);
        return;
    }

    // POST 请求
    if (mg_strcmp(hm->method, mg_str("POST")) == 0)
    {
        handle_api_post(c, hm);
    }
}
