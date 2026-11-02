#ifndef ESURFINGCLIENT_NETCLIENTINTERNAL_H
#define ESURFINGCLIENT_NETCLIENTINTERNAL_H

#include "net/NetClient.h"

#include "core/States.h"

#define URL_LENGTH 64

extern _Thread_local char g_request_url[LOCATION_LEN];

/* NetClientCurl.c */
size_t header_cb(const void* contents, size_t size, size_t nmemb, void* userdata);

size_t write_cb(const void* contents, size_t size, size_t nmemb, void* userdata);

char* calc_md5(const char* data);

network_status_t curl_err_msg_out(CURLcode curl_code);

void log_curl_error(CURL* curl, CURLcode code, const char* errbuf, const char* url, const char* func_name);

#ifdef __OPENWRT__
curl_socket_t open_socket_callback(void* client_p, curlsocktype purpose, struct curl_sockaddr* addr);
#endif

#endif
