//
// Created by bad_g on 2025/9/14.
//
#include "headFiles/NetworkStatus.h"
#include "headFiles/Constants.h"
#include "headFiles/States.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* memory;
    size_t size;
} HTTPResponse;

// �ص��������ڽ�����Ӧ����
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, HTTPResponse *response) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(response->memory, response->size + realsize + 1);
    if (!ptr) return 0;

    response->memory = ptr;
    memcpy(&(response->memory[response->size]), contents, realsize);
    response->size += realsize;
    response->memory[response->size] = 0;
    return realsize;
}

char* extract_between_tags(const char* text, const char* start_tag, const char* end_tag) {
    char* start = strstr(text, start_tag);
    if (!start) return NULL;

    start += strlen(start_tag);
    char* end = strstr(start, end_tag);
    if (!end) return NULL;

    size_t len = end - start;
    char* result = malloc(len + 1);
    if (!result) return NULL;

    strncpy(result, start, len);
    result[len] = '\0';
    return result;
}

// ����CDATA��ǩ�ĺ���
char* clean_cdata(const char* text) {
    if (!text) return NULL;
    
    const char* cdata_start = "<![CDATA[";
    const char* cdata_end = "]]>";
    
    char* start = strstr(text, cdata_start);
    if (!start) {
        // û��CDATA��ǩ��ֱ�Ӹ���ԭ�ı�
        return strdup(text);
    }
    
    start += strlen(cdata_start);
    char* end = strstr(start, cdata_end);
    if (!end) {
        // û���ҵ�������ǩ������ԭ�ı�
        return strdup(text);
    }
    
    size_t len = end - start;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    strncpy(result, start, len);
    result[len] = '\0';
    return result;
}

char* extract_xml_tag_content(const char* xml, const char* tag) {
    char start_tag[256], end_tag[256];
    snprintf(start_tag, sizeof(start_tag), "<%s>", tag);
    snprintf(end_tag, sizeof(end_tag), "</%s>", tag);

    return extract_between_tags(xml, start_tag, end_tag);
}

char* extract_url_parameter(const char* url, const char* param_name) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "%s=", param_name);

    char* param_start = strstr(url, search_pattern);
    if (!param_start) return NULL;

    param_start += strlen(search_pattern);
    char* param_end = strchr(param_start, '&');
    if (!param_end) param_end = param_start + strlen(param_start);

    size_t len = param_end - param_start;
    char* result = malloc(len + 1);
    if (!result) return NULL;

    strncpy(result, param_start, len);
    result[len] = '\0';
    return result;
}

ConnectivityStatus detectConfig(void) {
    CURL *curl;
    CURLcode res;
    long response_code = 0;
    HTTPResponse response_data = {0};

    curl = curl_easy_init();
    if (!curl) {
        return CONNECTIVITY_REQUEST_ERROR;
    }

    // ����URL�ͻ���ѡ��
    curl_easy_setopt(curl, CURLOPT_URL, CAPTIVE_URL);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // �����ض���
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);      // ����ض������

    // ��������ͷ
    struct curl_slist *headers = NULL;
    char header_buffer[256];

    snprintf(header_buffer, sizeof(header_buffer), "User-Agent: %s", USER_AGENT);
    headers = curl_slist_append(headers, header_buffer);

    snprintf(header_buffer, sizeof(header_buffer), "Accept: %s", REQUEST_ACCEPT);
    headers = curl_slist_append(headers, header_buffer);

    snprintf(header_buffer, sizeof(header_buffer), "Client-ID: %s", clientId);
    headers = curl_slist_append(headers, header_buffer);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // ������Ӧ���ݽ���
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    // ִ������
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        if (response_data.memory) free(response_data.memory);
        return CONNECTIVITY_REQUEST_ERROR;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    // ���HTTP״̬��
    if (response_code == 204) {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        if (response_data.memory) free(response_data.memory);
        return CONNECTIVITY_SUCCESS;
    }

    if (response_code != 200 && response_code != 302) {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        if (response_data.memory) free(response_data.memory);
        return CONNECTIVITY_REQUEST_ERROR;
    }

    // ������Ӧ����
    if (response_data.memory && response_data.size > 0) {
        // ��ȡ�Ż�����
        char* portal_config = extract_between_tags(response_data.memory,
            PORTAL_START_TAG, PORTAL_END_TAG);

        if (portal_config) {
        }

        if (portal_config && strlen(portal_config) > 0) {
            // ��ȡauth-url��ticket-url
            char* auth_url_raw = extract_xml_tag_content(portal_config, "auth-url");
            char* ticket_url_raw = extract_xml_tag_content(portal_config, "ticket-url");
            
            // ����CDATA��ǩ
            char* auth_url = clean_cdata(auth_url_raw);
            char* ticket_url = clean_cdata(ticket_url_raw);
            
            // �ͷ�ԭʼ����
            if (auth_url_raw) free(auth_url_raw);
            if (ticket_url_raw) free(ticket_url_raw);

            if (auth_url && ticket_url && strlen(auth_url) > 0 && strlen(ticket_url) > 0) {
                // ����ȫ��״̬ - ʹ�ö�̬�ڴ����
                if (authUrl) free(authUrl);
                authUrl = strdup(auth_url);
                
                if (ticketUrl) free(ticketUrl);
                ticketUrl = strdup(ticket_url);

                // ��ticket_url����ȡIP����
                char* user_ip = extract_url_parameter(ticket_url, "wlanuserip");
                char* ac_ip = extract_url_parameter(ticket_url, "wlanacip");

                if (user_ip && ac_ip) {
                    if (userIp) free(userIp);
                    userIp = strdup(user_ip);
                    
                    if (acIp) free(acIp);
                    acIp = strdup(ac_ip);

                    // �����ڴ�
                    free(user_ip);
                    free(ac_ip);
                    free(auth_url);
                    free(ticket_url);
                    free(portal_config);

                    curl_easy_cleanup(curl);
                    curl_slist_free_all(headers);
                    free(response_data.memory);

                    return CONNECTIVITY_REQUIRE_AUTHORIZATION;
                }

                if (user_ip) free(user_ip);
                if (ac_ip) free(ac_ip);
            }

            if (auth_url) free(auth_url);
            if (ticket_url) free(ticket_url);
            free(portal_config);
        }
    }

    // ������Դ
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    if (response_data.memory) free(response_data.memory);

    return CONNECTIVITY_SUCCESS;
}