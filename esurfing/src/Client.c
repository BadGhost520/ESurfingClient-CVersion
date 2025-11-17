//
// Created by bad_g on 2025/9/14.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headFiles/NetClient.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/NetworkStatus.h"
#include "headFiles/utils/Logger.h"

char* keepRetry;
char* keepUrl;
char* termUrl;
long long tick;

void term()
{
    const char* encrypt = sessionEncrypt(createXMLPayload("term"));
    LOG_DEBUG("Send encrypt: %s", encrypt);
    NetResult* result = simPost(termUrl, encrypt);
    if (result && result->type == NET_RESULT_ERROR)
    {
        LOG_ERROR("Log out error: %s", result->errorMessage ? result->errorMessage : "Unknown error");
    }
    freeNetResult(result);
}

void heartbeat()
{
    const char* encrypt = sessionEncrypt(createXMLPayload("heartbeat"));
    LOG_DEBUG("Send encrypt: %s", encrypt);
    NetResult* result = simPost(keepUrl, sessionEncrypt(createXMLPayload("heartbeat")));
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        char* decrypted_data = sessionDecrypt(result->data);
        if (decrypted_data) {
            LOG_DEBUG("result: %s", decrypted_data);
            char* parsed_interval = XmlParser(decrypted_data, "interval");
            if (parsed_interval) {
                free(keepRetry);
                keepRetry = parsed_interval;
            }
            free(decrypted_data);
        } else {
            LOG_ERROR("Failed to decrypt heartbeat response");
        }
    }
    else
    {
        LOG_ERROR("Heartbeat request failed");
    }
    freeNetResult(result);
}

void login()
{
    const char* encrypt = sessionEncrypt(createXMLPayload("login"));
    LOG_DEBUG("Send encrypt: %s", encrypt);
    NetResult* result = simPost(authUrl, sessionEncrypt(createXMLPayload("login")));
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        LOG_DEBUG("result: %s", result->data);
        char* decrypted_data = sessionDecrypt(result->data);
        if (!decrypted_data) {
            LOG_ERROR("Failed to decrypt login response");
            freeNetResult(result);
            return;
        }
        
        char* parsed_keep_retry = XmlParser(decrypted_data, "keep-retry");
        if (parsed_keep_retry) {
            free(keepRetry);
            keepRetry = parsed_keep_retry;
        }
        
        char* parsed_keep_url = cleanCDATA(XmlParser(decrypted_data, "keep-url"));
        if (parsed_keep_url) {
            free(keepUrl);
            keepUrl = parsed_keep_url;
        }
        
        char* parsed_term_url = cleanCDATA(XmlParser(decrypted_data, "term-url"));
        if (parsed_term_url) {
            free(termUrl);
            termUrl = parsed_term_url;
        }
        
        LOG_INFO("Keep Url: %s", keepUrl ? keepUrl : "NULL");
        LOG_INFO("Term Url: %s", termUrl ? termUrl : "NULL");
        LOG_INFO("Keep Retry: %s", keepRetry ? keepRetry : "NULL");
    }
    else
    {
        LOG_ERROR("Result is empty");
    }
    freeNetResult(result);
}

void getTicket()
{
    const char* encrypt = sessionEncrypt(createXMLPayload("getTicket"));
    LOG_DEBUG("Send encrypt: %s", encrypt);
    NetResult* result = simPost(ticketUrl, sessionEncrypt(createXMLPayload("getTicket")));
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        LOG_DEBUG("result: %s", result->data);
        char* parsed_ticket = XmlParser(sessionDecrypt(result->data), "ticket");
        if (parsed_ticket) {
            ticket = strdup(parsed_ticket);
            free(parsed_ticket);
        } else {
            LOG_ERROR("Failed to parse ticket from response");
        }
    }
    freeNetResult(result);
}

void initSession()
{
    NetResult* result = simPost(ticketUrl, algoId);
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        LOG_DEBUG("result: %s", result->data);
        const ByteArray zsm = stringToBytes(result->data);
        initialize(&zsm);
        free(zsm.data);
    } else {
        LOG_ERROR("Initialization session error");
    }
    freeNetResult(result);
}

void authorization()
{
    initSession();
    if (!isInitialized)
    {
        LOG_FATAL("Session initialization failed, please restart the application or download the application from Release again");
        LOG_FATAL("Release Url: https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest");
        isRunning = 0;
        return;
    }
    LOG_INFO("Client IP: %s", userIp);
    LOG_INFO("AC IP: %s", acIp);
    getTicket();
    LOG_INFO("Ticket: %s", ticket);
    login();
    if (keepUrl == NULL)
    {
        LOG_FATAL("Keep Url is empty");
        sessionFree();
        isRunning = 0;
        return;
    }
    tick = currentTimeMillis();
    isLogged = 1;
    LOG_INFO("Authorized login");
}

void run()
{
    switch (checkStatus())
    {
    case SUCCESS:
        if (isInitialized && isLogged)
        {
            long long keep_retry;
            if (stringToLongLong(keepRetry, &keep_retry))
            {
                if (currentTimeMillis() - tick >= keep_retry * 1000)
                {
                    LOG_INFO("Send heartbeat packet");
                    heartbeat();
                    LOG_INFO("Next retry: %ss", keepRetry);
                    tick = currentTimeMillis();
                }
            }
            else
            {
                LOG_ERROR("String to int64 error");
            }
        }
        else
        {
            LOG_INFO("The network is connected");
        }
        sleepMilliseconds(1000);
        break;
    case REQUIRE_AUTHORIZATION:
        LOG_INFO("authentication required");
        authorization();
        sleepMilliseconds(1000);
        break;
    case REQUEST_ERROR:
        LOG_ERROR("Network error");
        sleepMilliseconds(5000);
        break;
    default:
        LOG_ERROR("Unknown error");
        sleepMilliseconds(5000);
    }
}