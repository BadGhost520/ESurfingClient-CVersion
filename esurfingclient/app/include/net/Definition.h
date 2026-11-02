#ifndef ESURFINGCLIENT_DEFINITION_H
#define ESURFINGCLIENT_DEFINITION_H

#include <curl/curl.h>

#include <stdbool.h>

/** 响应头 Error-Code / Sub-Error 的长度 (服务端最长的形如 13012000) */
#define ERROR_CODE_LEN 24

#define HTTP_OK 200
#define HTTP_NO_CONTENT 204
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_FOUND 302

typedef enum {
    STATUS_OK = 0,
    STATUS_NEED_AUTH = 1,
    STATUS_ERROR = 2,
    STATUS_INIT_ERROR = 3,
} network_status_t;

typedef enum
{
    CONNECT_INTERNET = 0,
    CONNECT_AUTH_SERVER = 1,
    CONNECT_ERROR = 2,
} connection_status_t;

typedef struct {
    network_status_t status;
    long http_code;
    CURLcode curl_code;
    char* body_data;
    size_t body_size;
    char error_code[ERROR_CODE_LEN]; // 响应头 Error-Code (portal 侧错误码)
    char sub_error[ERROR_CODE_LEN];  // 响应头 Sub-Error (RADIUS/ACS 侧错误码)
} curl_resp_t;

#endif
