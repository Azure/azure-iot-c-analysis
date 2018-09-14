// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "network_info.h"
#include "mem_reporter.h"

#include "iothub_client.h"
#include "iothub_message.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/gbnetwork.h"

#include "iothubtransportmqtt.h"
#include "iothubtransportmqtt_websockets.h"
#include "iothubtransportamqp.h"
#include "iothubtransportamqp_websockets.h"
#include "iothubtransporthttp.h"

#include "../certs/certs.h"

#include "iothub_client_version.h"

#define PROXY_PORT                  8888
#define MESSAGES_TO_USE            1
#define TIME_BETWEEN_MESSAGES       1

typedef struct IOTHUB_CLIENT_SAMPLE_INFO_TAG
{
    int connected;
    int stop_running;
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
    if (protocol == PROTOCOL_HTTP)
    {
        result = 0;
    }
    else
    {
        IOTHUB_CLIENT_TRANSPORT_PROVIDER iothub_transport;
        TICK_COUNTER_HANDLE tick_counter_handle;
        MEM_ANALYSIS_INFO iot_mem_info;
        memset(&iot_mem_info, 0, sizeof(MEM_ANALYSIS_INFO));

        gbnetwork_resetMetrics();

        if ((iothub_transport = initialize(&iot_mem_info, protocol, num_msgs_to_send)) == NULL)
        {
            (void)printf("Failed setting transport failed\r\n");
            result = __LINE__;
        }
        else if ((tick_counter_handle = tickcounter_create()) == NULL)
        {
            (void)printf("tickcounter_create failed\r\n");
            result = __LINE__;
        }
        else
        {
            gballoc_resetMetrics();
            iot_mem_info.operation_type = OPERATION_NETWORK;

            // Sending the iothub messages
            IOTHUB_CLIENT_LL_HANDLE iothub_client;
            if ((iothub_client = IoTHubClient_LL_CreateFromConnectionString(conn_info->device_conn_string, iothub_transport)) == NULL)
            {
                (void)printf("failed create IoTHub client from connection string %s!\r\n", conn_info->device_conn_string);
                result = __LINE__;
            }
            else
            {
                result = 0;
                IOTHUB_CLIENT_INFO iothub_info;
                tickcounter_ms_t current_tick;
                tickcounter_ms_t last_send_time = TIME_BETWEEN_MESSAGES;
                size_t msg_count = 0;
                iothub_info.stop_running = 0;
                iothub_info.connected = 0;

                if (protocol == PROTOCOL_HTTP)
                {
                    iothub_info.connected = 1;
                }

                (void)IoTHubClient_LL_SetConnectionStatusCallback(iothub_client, iothub_connection_status, &iothub_info);

                // Set the certificate
                IoTHubClient_LL_SetOption(iothub_client, OPTION_TRUSTED_CERT, certificates);
                do
                {
                    if (iothub_info.connected != 0)
                    {
                        // Send a message every TIME_BETWEEN_MESSAGES seconds
                        (void)tickcounter_get_current_ms(tick_counter_handle, &current_tick);
                        if ((current_tick - last_send_time) / 1000 > TIME_BETWEEN_MESSAGES)
                        {
                            IOTHUB_MESSAGE_HANDLE msg_handle;
                            static char msgText[1024];
                            sprintf_s(msgText, sizeof(msgText), "{ \"message_index\" : \"%zu\" }", msg_count++);

                            iot_mem_info.msg_sent = strlen(msgText);
                            if (use_byte_array_msg)
                            {
                                msg_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, iot_mem_info.msg_sent);
                            }
                            else
                            {
                                msg_handle = IoTHubMessage_CreateFromString(msgText);
                            }
                            if (msg_handle == NULL)
                            {
                                (void)printf("ERROR: iotHubMessageHandle is NULL!\r\n");
                            }
                            else
                            {
                                (void)IoTHubMessage_SetMessageId(msg_handle, "MSG_ID");
                                (void)IoTHubMessage_SetCorrelationId(msg_handle, "CORE_ID");
                                (void)IoTHubMessage_SetContentTypeSystemProperty(msg_handle, "application%2Fjson");
                                (void)IoTHubMessage_SetContentEncodingSystemProperty(msg_handle, "utf-8");

                                MAP_HANDLE propMap = IoTHubMessage_Properties(msg_handle);
                                if (Map_AddOrUpdate(propMap, "property", "property_text") != MAP_OK)
                                {
                                    (void)printf("ERROR: Map_AddOrUpdate Failed!\r\n");
                                }

                                if (IoTHubClient_LL_SendEventAsync(iothub_client, msg_handle, NULL, NULL) != IOTHUB_CLIENT_OK)
                                {
                                    (void)printf("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED!\r\n");
                                }
                                else
                                {
                                    (void)tickcounter_get_current_ms(tick_counter_handle, &last_send_time);
                                }
                                IoTHubMessage_Destroy(msg_handle);
                            }
                        }
                    }
                    IoTHubClient_LL_DoWork(iothub_client);
                    ThreadAPI_Sleep(1);
                } while (iothub_info.stop_running == 0 && msg_count < num_msgs_to_send);

                size_t index = 0;
                for (index = 0; index < 10; index++)
                {
                    IoTHubClient_LL_DoWork(iothub_client);
                    ThreadAPI_Sleep(1);
                }
                IoTHubClient_LL_Destroy(iothub_client);

                report_network_usage(NULL, &iot_mem_info);
            }
            tickcounter_destroy(tick_counter_handle);
        }
    }
    return result;
}

int initiate_upper_level_operation(const CONNECTION_INFO* conn_info, PROTOCOL_TYPE protocol, size_t num_msgs_to_send, bool use_byte_array_msg)
{
    (void)conn_info;
    (void)protocol;
    (void)num_msgs_to_send;
    (void)use_byte_array_msg;
    int result = 0;
    return result;
}
