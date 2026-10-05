#ifndef ESURFINGCLIENT_WEBINTERNAL_H
#define ESURFINGCLIENT_WEBINTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <mongoose/mongoose.h>

extern bool s_local_state;

#define LOG_NAME_LEN 256

#define HEADER_JSON "Content-Type: application/json\r\nCache-Control: no-store\r\n"
#define HEADER_TEXT "Content-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\n"
#define HEADER_TEXT_TRUNCATED HEADER_TEXT "X-Log-Truncated: 1\r\n"

typedef struct
{
    char name[LOG_NAME_LEN];
    uint64_t size;
    uint64_t mtime;
    bool current;
} log_file_entry_t;

/* WebApiGet.c */
bool query_is_authed(bool* out);

bool handle_api_get(struct mg_connection* c, struct mg_http_message* hm);

/* WebApiPost.c */
bool handle_api_post(struct mg_connection* c, struct mg_http_message* hm);

/* WebLogFiles.c */
void format_week_min(uint16_t week_min, char* out);

bool is_safe_log_name(const char* name);

bool is_log_file_name(const char* name);

int list_log_files(log_file_entry_t* out, int max);

int cmp_log_files(const void* a, const void* b);

void logFn(char ch, void* param);

/* WebServe.c */
int str_case_cmp(const char* a, const char* b);

#ifdef _WIN32
bool win_stat_file(const char* path, uint64_t* size, uint64_t* mtime);
#endif

void fn(struct mg_connection* c, int ev, void* ev_data);

#endif
