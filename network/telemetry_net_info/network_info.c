// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "network_info.h"
#include "mem_reporter.h"

#include "iothub_client_version.h"
#include "iothub_device_client_ll.h"
#include "iothub_message.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/gbnetwork.h"

#ifdef USE_MQTT
    #include "iothubtransportmqtt.h"
    #include "iothubtransportmqtt_websockets.h"
#endif

#ifdef USE_AMQP
    #include "iothubtransportamqp.h"
    #include "iothubtransportamqp_websockets.h"
#endif

#include "../certs/certs.h"

#define PROXY_PORT              8888
#define MESSAGES_TO_USE         1
#define TIME_BETWEEN_MESSAGES   1
#define MESSAGE_PAYLOAD_LENGTH  962

typedef struct IOTHUB_CLIENT_SAMPLE_INFO_TAG
{
    int connected;
    int stop_running;
    int exclude_conn_header;
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
        case PROTOCOL_AMQP:
            result = AMQP_Protocol;
            break;
        case PROTOCOL_AMQP_WS:
            result = AMQP_Protocol_over_WebSocketsTls;
            break;
        case PROTOCOL_HTTP:
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
            // Reset the Metrics so to not get the connection preamble included, just the sends
            if (iothub_info->exclude_conn_header == 0)
            {
                gbnetwork_resetMetrics();
            }
        }
        else
        {
            iothub_info->connected = 0;
            iothub_info->stop_running = 1;
        }
    }
}

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* user_context)
{
    if (user_context == NULL)
    {
        (void)printf("iothub_connection_status user_context is NULL\r\n");
    }
    else
    {
        IOTHUB_CLIENT_INFO* iothub_info = (IOTHUB_CLIENT_INFO*)user_context;
        iothub_info->stop_running = 1;
    }
}

int initiate_lower_level_operation(const CONNECTION_INFO* conn_info, REPORT_HANDLE report_handle, PROTOCOL_TYPE protocol, size_t num_msgs_to_send, bool use_byte_array_msg, int exclude_conn_header)
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
            IOTHUB_DEVICE_CLIENT_LL_HANDLE device_client;
            if ((device_client = IoTHubDeviceClient_LL_CreateFromConnectionString(conn_info->device_conn_string, iothub_transport)) == NULL)
            {
                (void)printf("failed create IoTHub client from connection string %s!\r\n", conn_info->device_conn_string);
                result = __LINE__;
            }
            else
            {
                result = 0;
                IOTHUB_CLIENT_INFO iothub_info;
                size_t msg_count = 0;
                iothub_info.stop_running = 0;
                iothub_info.connected = 0;
                iothub_info.exclude_conn_header = exclude_conn_header;

                if (protocol == PROTOCOL_HTTP)
                {
                    iothub_info.connected = 1;
                }

                (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_client, iothub_connection_status, &iothub_info);

                // Set the certificate
                IoTHubDeviceClient_LL_SetOption(device_client, OPTION_TRUSTED_CERT, certificates);
                do
                {
                    if (iothub_info.connected != 0)
                    {
                        // Send a message every TIME_BETWEEN_MESSAGES seconds
                        if (msg_count < num_msgs_to_send)
                        {
                            IOTHUB_MESSAGE_HANDLE msg_handle;
                            static char msgText[MESSAGE_PAYLOAD_LENGTH + 1];
                            memset(msgText, 0, MESSAGE_PAYLOAD_LENGTH + 1);
                            char current_value = 'a';
                            for (size_t index = 0; index < MESSAGE_PAYLOAD_LENGTH; index++)
                            {
                                msgText[index] = current_value;
                                if (current_value == 'z')
                                {
                                    current_value = 'a';
                                }
                                else
                                {
                                    current_value++;
                                }
                            }

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
                                iot_mem_info.msg_sent += strlen("MSG_ID");
                                (void)IoTHubMessage_SetCorrelationId(msg_handle, "CORE_ID");
                                iot_mem_info.msg_sent += strlen("CORE_ID");
                                (void)IoTHubMessage_SetContentTypeSystemProperty(msg_handle, "application%2Fjson");
                                iot_mem_info.msg_sent += strlen("application%2Fjson");
                                (void)IoTHubMessage_SetContentEncodingSystemProperty(msg_handle, "utf-8");
                                iot_mem_info.msg_sent += strlen("utf-8");

                                (void)IoTHubMessage_SetProperty(msg_handle, "property_key", "property_value");
                                iot_mem_info.msg_sent += strlen("property_key");
                                iot_mem_info.msg_sent += strlen("property_value");

                                if (IoTHubDeviceClient_LL_SendEventAsync(device_client, msg_handle, send_confirm_callback, &iothub_info) != IOTHUB_CLIENT_OK)
                                {
                                    (void)printf("ERROR: IoTHubDeviceClient_LL_SendEventAsync..........FAILED!\r\n");
                                }
                                else
                                {
                                    msg_count++;
                                }
                                IoTHubMessage_Destroy(msg_handle);
                            }
                        }
                    }
                    IoTHubDeviceClient_LL_DoWork(device_client);
                    ThreadAPI_Sleep(1);
                } while (iothub_info.stop_running == 0);

                size_t index = 0;
                for (index = 0; index < 10; index++)
                {
                    IoTHubDeviceClient_LL_DoWork(device_client);
                    ThreadAPI_Sleep(1);
                }
                IoTHubDeviceClient_LL_Destroy(device_client);

                report_network_usage(report_handle, &iot_mem_info);
            }
            tickcounter_destroy(tick_counter_handle);
        }
    }
    return result;
}
