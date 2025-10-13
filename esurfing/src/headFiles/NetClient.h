//
// Created by bad_g on 2025/9/23.
//

#ifndef ESURFINGCLIENT_NETCLIENT_H
#define ESURFINGCLIENT_NETCLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

    // 最大长度定义
#define MAX_URL_LENGTH 512
#define MAX_HEADER_LENGTH 256
#define MAX_DATA_LENGTH 4096
#define MAX_RESPONSE_LENGTH 8192
#define MAX_HEADERS_COUNT 20

    // NetResult结构体 - 对应Kotlin的NetResult，支持二进制数据
    typedef enum {
        NET_RESULT_SUCCESS,
        NET_RESULT_ERROR
    } NetResultType;

    typedef struct {
        NetResultType type;
        char* data;           // 响应数据（可能包含二进制数据）
        size_t data_size;     // 数据大小（字节数）
        char* error_message;  // 失败时的错误信息
        int status_code;      // HTTP状态码
    } NetResult;

    // 额外请求头结构体 - 对应Kotlin的HashMap<String, String>
    typedef struct {
        char key[MAX_HEADER_LENGTH];
        char value[MAX_HEADER_LENGTH];
    } HeaderPair;

    typedef struct {
        HeaderPair headers[MAX_HEADERS_COUNT];
        int count;
    } ExtraHeaders;

    // 响应数据结构体
    typedef struct {
        char* memory;
        size_t size;
    } ResponseData;

    // 函数声明
    NetResult* post_request(const char* url, const char* data, ExtraHeaders* extra_headers);
    void free_net_result(NetResult* result);
    void init_extra_headers(ExtraHeaders* headers);
    void add_extra_header(ExtraHeaders* headers, const char* key, const char* value);
    char* calculate_md5(const char* data);
    size_t write_response_callback(void* contents, size_t size, size_t nmemb, ResponseData* response);
    NetResult* simplePost(const char* url, const char* data);

    // 初始化和清理函数 (现在是可选的，POST函数会自动处理)
    int init_post_client(void);           // 可选：手动初始化客户端
    void cleanup_post_client(void);       // 可选：手动清理客户端 (程序退出时自动清理)

#ifdef __cplusplus
}
#endif

#endif //ESURFINGCLIENT_NETCLIENT_H