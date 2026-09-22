#ifndef ESURFINGCLIENT_DEFINITION_H
#define ESURFINGCLIENT_DEFINITION_H

#include <curl/curl.h>

#include <stdbool.h>

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
} curl_resp_t;

#endif //ESURFINGCLIENT_DEFINITION_H
