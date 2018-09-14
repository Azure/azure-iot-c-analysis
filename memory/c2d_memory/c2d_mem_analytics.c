// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "c2d_mem_analytics.h"

#include "iothub_client.h"
#include "iothub_message.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/gballoc.h"

#include "iothubtransportmqtt.h"
#include "iothubtransportmqtt_websockets.h"
#include "iothubtransportamqp.h"
#include "iothubtransportamqp_websockets.h"
#include "iothubtransporthttp.h"

#include "../certs/certs.h"

#include "iothub_client_version.h"

#define PROXY_PORT                  8888
#define TIME_BETWEEN_MESSAGES       1

typedef struct IOTHUB_CLIENT_INFO_TAG
{
    int connected;
    int stop_running;
    size_t msg_count;
    bool use_byte_array;
} IOTHUB_CLIENT_INFO;

static IOTHUB_CLIENT_TRANSPORT_PROVIDER initialize(MEM_ANALYSIS_INFO* iot_mem_info, PROTOCOL_TYPE protocol, size_t num_msgs_to_send)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER result;
    iot_mem_info->msg_sent = num_msgs_to_send;
    iot_mem_info->iothub_version = IoTHubClient_GetVersionString();

    iot_mem_info->iothub_protocol = protocol;
    switch (protocol)
    {
        case PROTOCOL_MQTT:
            result = MQTT_Protocol;
            break;
        case PROTOCOL_MQTT_WS:
            result = MQTT_WebSocket_Protocol;
            break;
        case PROTOCOL_HTTP:
            result = HTTP_Protocol;
            break;
        case PROTOCOL_AMQP:
            result = AMQP_Protocol;
            break;
        case PROTOCOL_AMQP_WS:
            result = AMQP_Protocol_over_WebSocketsTls;
            break;
        default:
            result = NULL;
            break;
    }
    return result;
}

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    IOTHUB_CLIENT_INFO* iot_client_info = (IOTHUB_CLIENT_INFO*)user_context;

    IOTHUBMESSAGE_CONTENT_TYPE content_type = IoTHubMessage_GetContentType(message);
    if (content_type == IOTHUBMESSAGE_BYTEARRAY)
    {
        const unsigned char* buff_msg;
        size_t buff_len;

        if (IoTHubMessage_GetByteArray(message, &buff_msg, &buff_len) != IOTHUB_MESSAGE_OK)
        {
            (void)printf("Failure retrieving byte array message\r\n");
        }
    }
    else
    {
        const char* string_msg = IoTHubMessage_GetString(message);
        if (string_msg == NULL)
        {
            (void)printf("Failure retrieving byte array message\r\n");
        }
    }
    iot_client_info->msg_count++;

    return IOTHUBMESSAGE_ACCEPTED;
}

static void iothub_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    if (user_context == NULL)
    {
        (void)printf("iothub_connection_status user_context is NULL\r\n");
    }
    else
    {
        IOTHUB_CLIENT_INFO* iothub_info = (IOTHUB_CLIENT_INFO*)user_context;
        if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
        {
            iothub_info->connected = 1;
        }
        else
        {
            iothub_info->connected = 0;
            iothub_info->stop_running = 1;
        }
    }
}

int initiate_lower_level_operation(const CONNECTION_INFO* conn_info, PROTOCOL_TYPE protocol, size_t num_msgs_to_send, bool use_byte_array_msg)
{
    int result;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER iothub_transport;

    MEM_ANALYSIS_INFO iot_mem_info;
    memset(&iot_mem_info, 0, sizeof(MEM_ANALYSIS_INFO));

    if ((iothub_transport = initialize(&iot_mem_info, protocol, num_msgs_to_send)) == NULL)
    {
        (void)printf("Failed setting transport failed\r\n");
        result = __LINE__;
    }
    else
    {
        gballoc_resetMetrics();
        iot_mem_info.operation_type = OPERATION_MEMORY;
        iot_mem_info.feature_type = FEATURE_C2D_LL;

        // Sending the iothub messages
        IOTHUB_CLIENT_LL_HANDLE iothub_client;
        if ((iothub_client = IoTHubClient_LL_CreateFromConnectionString(conn_info->device_conn_string, iothub_transport) ) == NULL)
        {
            (void)printf("failed create IoTHub client from connection string %s!\r\n", conn_info->device_conn_string);
            result = __LINE__;
        }
        else
        {
            IOTHUB_CLIENT_INFO iothub_info;
            memset(&iothub_info, 0, sizeof(IOTHUB_CLIENT_INFO));

            if (protocol == PROTOCOL_HTTP)
            {
                iothub_info.connected = 1;
            }

            (void)IoTHubClient_LL_SetConnectionStatusCallback(iothub_client, iothub_connection_status, &iothub_info);

            //IoTHubClient_SetOption(iothub_client, "logtrace", &g_trace_on);
            // Always set the cert so we can compare apples to apples
            IoTHubClient_LL_SetOption(iothub_client, OPTION_TRUSTED_CERT, certificates);

            if (IoTHubClient_LL_SetMessageCallback(iothub_client, receive_msg_callback, &iothub_info) != IOTHUB_CLIENT_OK)
            {
                (void)printf("ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");
                result = __LINE__;
            }
            else
            {
                bool send_msg = true;
                result = 0;
                do
                {
                    if (iothub_info.connected == 1 && send_msg)
                    {
                        // Send message to the client
                        (void)printf("Send Message Now\r\n");
                        send_msg = false;
                    }
                    IoTHubClient_LL_DoWork(iothub_client);
                    ThreadAPI_Sleep(20);
                } while (iothub_info.stop_running != 0 || iothub_info.msg_count < num_msgs_to_send);
            }

            IoTHubClient_LL_Destroy(iothub_client);

            report_memory_usage(NULL, &iot_mem_info);
        }
    }
    return result;
}

int initiate_upper_level_operation(const CONNECTION_INFO* conn_info, PROTOCOL_TYPE protocol, size_t num_msgs_to_send, bool use_byte_array_msg)
{
    int result;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER iothub_transport;

    MEM_ANALYSIS_INFO iot_mem_info;
    memset(&iot_mem_info, 0, sizeof(MEM_ANALYSIS_INFO));

    if ((iothub_transport = initialize(&iot_mem_info, protocol, num_msgs_to_send)) == NULL)
    {
        (void)printf("Failed setting transport failed\r\n");
        result = __LINE__;
    }
    else
    {
        gballoc_resetMetrics();

        iot_mem_info.operation_type = OPERATION_MEMORY;
        iot_mem_info.feature_type = FEATURE_C2D_UL;

        // Sending the iothub messages
        IOTHUB_CLIENT_HANDLE iothub_client;
        if ((iothub_client = IoTHubClient_CreateFromConnectionString(conn_info->device_conn_string, iothub_transport)) == NULL)
        {
            (void)printf("failed create IoTHub client from connection string %s!\r\n", conn_info->device_conn_string);
            result = __LINE__;
        }
        else
        {
            IOTHUB_CLIENT_INFO iothub_info;
            memset(&iothub_info, 0, sizeof(IOTHUB_CLIENT_INFO));

            // Http doesn't have a connection callback
            if (protocol == PROTOCOL_HTTP)
            {
                iothub_info.connected = 1;
            }

            (void)IoTHubClient_SetConnectionStatusCallback(iothub_client, iothub_connection_status, &iothub_info);

            //IoTHubClient_SetOption(iothub_client, "logtrace", &g_trace_on);
            // Always set the cert so we can compare apples to apples
            IoTHubClient_SetOption(iothub_client, OPTION_TRUSTED_CERT, certificates);

            if (IoTHubClient_SetMessageCallback(iothub_client, receive_msg_callback, &iothub_info) != IOTHUB_CLIENT_OK)
            {
                (void)printf("ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");
                result = __LINE__;
            }
            else
            {
                bool send_msg = true;
                result = 0;
                do
                {
                    if (iothub_info.connected == 1 && send_msg)
                    {
                        // Send message to the client
                        (void)printf("Send Message Now\r\n");
                        send_msg = false;
                    }
                    ThreadAPI_Sleep(1000);
                } while (iothub_info.stop_running != 0 || iothub_info.msg_count < num_msgs_to_send);
            }

            IoTHubClient_Destroy(iothub_client);

            report_memory_usage(NULL, &iot_mem_info);
        }
    }
    return result;
}
