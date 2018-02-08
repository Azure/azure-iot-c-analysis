// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <vld.h>

#include <stdio.h>
#include <stdlib.h>

#include "sdk_mem_analytics.h"

#include "iothub_client.h"
#include "iothub_message.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/gballoc.h"

#ifdef USE_MQTT
    #include "iothubtransportmqtt.h"
    #include "iothubtransportmqtt_websockets.h"
#endif
#ifdef USE_AMQP
    #include "iothubtransportamqp.h"
    #include "iothubtransportamqp_websockets.h"
#endif
#ifdef USE_HTTP
    #include "iothubtransporthttp.h"
#endif

#include "../../../certs/certs.h"

#include "iothub_client_version.h"

#define PROXY_PORT                  8888
#define MESSAGES_TO_SEND            1
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

    switch (protocol)
    {
#ifdef USE_MQTT
    case PROTOCOL_MQTT:
        result = MQTT_Protocol;
        iot_mem_info->iothub_protocol = MQTT_PROTOCOL_NAME;
        break;
    case PROTOCOL_MQTT_WS:
        result = MQTT_WebSocket_Protocol;
        iot_mem_info->iothub_protocol = MQTT_WS_PROTOCOL_NAME;
        break;
#endif
#ifdef USE_HTTP
    case PROTOCOL_HTTP:
        result = HTTP_Protocol;
        iot_mem_info->iothub_protocol = HTTP_PROTOCOL_NAME;
        break;
#endif
#ifdef USE_AMQP
    case PROTOCOL_AMQP:
        result = AMQP_Protocol;
        iot_mem_info->iothub_protocol = AMQP_PROTOCOL_NAME;
        break;
    case PROTOCOL_AMQP_WS:
        result = AMQP_Protocol_over_WebSocketsTls;
        iot_mem_info->iothub_protocol = AMQP_WS_PROTOCOL_NAME;
        break;
#endif
    default:
        result = NULL;
        break;
    }
    return result;
}

static int send_device_method_message()
{
    int result;
    return result;
}

static int device_method_callback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* resp_size, void* user_context)
{
    int status = 200;
    if (user_context != NULL)
    {
        IOTHUB_CLIENT_INFO* iothub_info = (IOTHUB_CLIENT_INFO*)user_context;

        printf("\r\nDevice Method called\r\n");
        printf("Device Method name:    %s\r\n", method_name);
        printf("Device Method payload: %.*s\r\n", (int)size, (const char*)payload);

        const char* RESPONSE_STRING = "{ \"Response\": \"This is the response from the device\" }";

        *resp_size = strlen(RESPONSE_STRING);
        if ((*response = malloc(*resp_size)) == NULL)
        {
            status = -1;
        }
        else
        {
            (void)memcpy(*response, RESPONSE_STRING, *resp_size);
        }
        iothub_info->stop_running = 1;
    }
    return status;
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

int send_lower_level_operation(const char* conn_string, PROTOCOL_TYPE protocol, size_t num_msgs_to_send, bool use_byte_array_msg, memory_usage_callback memory_cb)
{
    int result;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER iothub_transport;

    TICK_COUNTER_HANDLE g_tick_counter_handle;
    MEM_ANALYSIS_INFO iot_mem_info;
    memset(&iot_mem_info, 0, sizeof(MEM_ANALYSIS_INFO));

    if ((iothub_transport = initialize(&iot_mem_info, protocol, num_msgs_to_send)) == NULL)
    {
        (void)printf("Failed setting transport failed\r\n");
        result = __LINE__;
    }
    else if ((g_tick_counter_handle = tickcounter_create()) == NULL)
    {
        (void)printf("tickcounter_create failed\r\n");
        result = __LINE__;
    }
    else
    {
        gballoc_resetMetrics();
        iot_mem_info.operation_type = OPERATION_METHODS;

        IOTHUB_CLIENT_INFO iothub_info;
        memset(&iothub_info, 0, sizeof(IOTHUB_CLIENT_INFO));

        // Sending the iothub messages
        IOTHUB_CLIENT_LL_HANDLE iothub_client;
        if ((iothub_client = IoTHubClient_LL_CreateFromConnectionString(conn_string, iothub_transport) ) == NULL)
        {
            (void)printf("failed create IoTHub client from connection string %s!\r\n", conn_string);
            result = __LINE__;
        }
        else if (IoTHubClient_LL_SetDeviceMethodCallback(iothub_client, device_method_callback, &iothub_info) != IOTHUB_CLIENT_OK)
        {
            (void)printf("failed setting device method callback!\r\n");
            result = __LINE__;
        }
        else
        {
            result = 0;
            int sent_device_method_msg = 0;

            (void)IoTHubClient_LL_SetOption(iothub_client, OPTION_TRUSTED_CERT, certificates);

            
            do
            {
                if (iothub_info.connected == 1 && sent_device_method_msg == 0)
                {
                    if (send_device_method_message() == 0)
                    {
                        sent_device_method_msg = 1;
                    }
                }
                IoTHubClient_LL_DoWork(iothub_client);
                ThreadAPI_Sleep(1);
            } while (iothub_info.stop_running == 0);

            size_t index = 0;
            for (index = 0; index < 10; index++)
            {
                IoTHubClient_LL_DoWork(iothub_client);
                ThreadAPI_Sleep(1);
            }
            IoTHubClient_LL_Destroy(iothub_client);

            memory_cb(&iot_mem_info);
        }
        tickcounter_destroy(g_tick_counter_handle);
    }
    return result;
}

int send_upper_level_operation(const char* conn_string, PROTOCOL_TYPE protocol, size_t num_msgs_to_send, bool use_byte_array_msg, memory_usage_callback mem_cb)
{
    int result;
    (void)use_byte_array_msg;
    (void)protocol;
    if (conn_string == NULL || num_msgs_to_send == 0 || mem_cb == NULL)
    {
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return 0;
}
