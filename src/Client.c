//
// Created by bad_g on 2025/9/14.
//
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "headFiles/NetClient.h"
#include "headFiles/PlatformUtils.h"
#include "headFiles/Session.h"
#include "headFiles/States.h"
#include "headFiles/NetworkStatus.h"
#include "headFiles/utils/ByteArray.h"
#include "headFiles/utils/XMLParser.h"
#include "headFiles/utils/Logger.h"
#include "headFiles/utils/ShutdownHook.h"

char* keepRetry;
char* keepUrl;
char* termUrl;
long long tick;
int retry = 0;

void authorization();

void term()
{
    NetResult* result = simplePost(termUrl, sessionEncrypt(createXMLPayload("term")));
    if (result->type == NET_RESULT_ERROR)
    {
        LOG_ERROR("Log out error");
    }
    free_net_result(result);
}

void heartbeat()
{
    NetResult* result = simplePost(keepUrl, sessionEncrypt(createXMLPayload("heartbeat")));
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        free(keepRetry);
        keepRetry = XML_Parser(sessionDecrypt(result->data), "interval");
    }
    else
    {
        LOG_ERROR("Result is empty");
    }
    free_net_result(result);
}

void login()
{
    NetResult* result = simplePost(authUrl, sessionEncrypt(createXMLPayload("login")));
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        keepRetry = XML_Parser(sessionDecrypt(result->data), "keep-retry");
        keepUrl = cleanCDATA(XML_Parser(sessionDecrypt(result->data), "keep-url"));
        termUrl = cleanCDATA(XML_Parser(sessionDecrypt(result->data), "term-url"));
        LOG_INFO("Keep Url: %s", keepUrl);
        LOG_INFO("Term Url: %s", termUrl);
        LOG_INFO("Keep Retry: %s", keepRetry);
    }
    else
    {
        LOG_ERROR("Result is empty");
    }
    free_net_result(result);
}

char* getTicket()
{
    NetResult* result = simplePost(ticketUrl, sessionEncrypt(createXMLPayload("getTicket")));
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        free_net_result(result);
        return XML_Parser(sessionDecrypt(result->data), "ticket");
    }
    LOG_ERROR("Result is empty");
    free_net_result(result);
    return NULL;
}

void initSession()
{
    NetResult* result = simplePost(ticketUrl, algoId);
    if (result && result->type == NET_RESULT_SUCCESS)
    {
        const ByteArray zsm = string_to_bytes(result->data);
        if (retry == 5)
        {
            LOG_ERROR("The Algo ID has failed to match multiple times. Please get the campus network IP address and then open this program again");
            graceful_exit(1);
        }
        if (initialize(&zsm) == 2 && retry < 5)
        {
            LOG_WARN("Algo ID matching failed, restarting program");
            retry++;
            free_net_result(result);
            free(zsm.data);
            sleepSeconds(1);
            authorization();
        }
        retry = 0;
        free(zsm.data);
    } else {
        LOG_ERROR("Initialization session error");
    }
    free_net_result(result);
}

void authorization()
{
    initSession();

    if (!isInitialized())
    {
        LOG_ERROR("Session initialization failed, please restart the application or retrieve the application from Release again");
        LOG_ERROR("Release Url: https://github.com/liu23zhi/ESurfingClient-CVersion/releases");
        isRunning = false;
        return;
    }

    LOG_INFO("Client IP: %s", userIp);
    LOG_INFO("AC IP: %s", acIp);

    ticket = getTicket();
    LOG_INFO("Ticket: %s", ticket);

    login();
    if (keepUrl == NULL)
    {
        LOG_ERROR("Keep Url is empty");
        sessionFree();
        isRunning = false;
        return;
    }

    tick = currentTimeMillis();
    isLogged = true;
    LOG_INFO("Authorized login");
}

void run()
{
    while (isRunning)
    {
        const ConnectivityStatus networkStatus = detectConfig();
        switch (networkStatus)
        {
        case CONNECTIVITY_SUCCESS:
            if (isInitialized() && isLogged)
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
                LOG_INFO("The network is connected.");
            }
            sleepSeconds(1);
            break;
        case CONNECTIVITY_REQUIRE_AUTHORIZATION:
            LOG_INFO("authentication required");
            isLogged = false;
            authorization();
            break;
        case CONNECTIVITY_REQUEST_ERROR:
            LOG_ERROR("Network error");
            sleepSeconds(5);
            break;
        default:
            LOG_ERROR("Unknown error");
            sleepSeconds(5);
        }
    }
}