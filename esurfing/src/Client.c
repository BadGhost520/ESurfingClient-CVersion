//
// Created by bad_g on 2025/9/14.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headFiles/Constants.h"
#include "headFiles/NetClient.h"
#include "headFiles/utils/PlatformUtils.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/NetworkStatus.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/utils/Shutdown.h"

char* keepRetry;
char* keepUrl;
char* termUrl;
long long tick;

void term()
{
    const char* encrypt = sessionEncrypt(createXMLPayload("term"));
    LOG_DEBUG("Send encrypt: %s", encrypt);
    NetResult* result = simPost(termUrl, encrypt);
    if (result->type == NET_RESULT_ERROR)
    {
        LOG_ERROR("Log out error");
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
        free(keepRetry);
        LOG_DEBUG("result: %s", result->data);
        keepRetry = XmlParser(sessionDecrypt(result->data), "interval");
    }
    else
    {
        LOG_ERROR("Result is empty");
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
        keepRetry = XmlParser(sessionDecrypt(result->data), "keep-retry");
        keepUrl = cleanCDATA(XmlParser(sessionDecrypt(result->data), "keep-url"));
        termUrl = cleanCDATA(XmlParser(sessionDecrypt(result->data), "term-url"));
        LOG_INFO("Keep Url: %s", keepUrl);
        LOG_INFO("Term Url: %s", termUrl);
        LOG_INFO("Keep Retry: %s", keepRetry);
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
        ticket = strdup(XmlParser(sessionDecrypt(result->data), "ticket"));
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
        LOG_ERROR("Session initialization failed, please restart the application or retrieve the application from Release again");
        LOG_ERROR("Release Url: https://github.com/BadGhost520/ESurfingClient-CVersion/releases");
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
        LOG_ERROR("Keep Url is empty");
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
    int count = 0;
    while (isRunning)
    {
        const ConnectivityStatus networkStatus = checkStatus();
        switch (networkStatus)
        {
        case CONNECTIVITY_SUCCESS:
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
        case CONNECTIVITY_REQUIRE_AUTHORIZATION:
            LOG_INFO("authentication required");
            isLogged = 0;
            if (count == 5)
            {
                LOG_ERROR("Unknown error, trying to fix");
                initConstants();
                refreshStates();
            }
            if (count == 6)
            {
                LOG_ERROR("Unknown error");
                shut(1);
            }
            count++;
            authorization();
            sleepMilliseconds(1000);
            break;
        case CONNECTIVITY_REQUEST_ERROR:
            LOG_ERROR("Network error");
            sleepMilliseconds(5000);
            break;
        default:
            LOG_ERROR("Unknown error");
            sleepMilliseconds(5000);
        }
    }
}