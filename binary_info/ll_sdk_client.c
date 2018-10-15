// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/gballoc.h"

#include "azure_c_shared_utility/shared_util_options.h"

#ifdef USE_MQTT
    #ifdef USE_WEB_SOCKETS
        #include "iothubtransportmqtt_websockets.h"
    #else
        #include "iothubtransportmqtt.h"
    #endif
#elif USE_AMQP
    #ifdef USE_WEB_SOCKETS
        #include "iothubtransportamqp_websockets.h"
    #else
        #include "iothubtransportamqp.h"
    #endif
#else // USE_HTTP
    #include "iothubtransporthttp.h"
#endif

#include "iothub_client_options.h"
#include "certs.h"

/* String containing Hostname, Device Id & Device Key in the format:                         */
/* Paste in the your iothub connection string  */
static const char* connectionString = "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<key>";

#define MESSAGE_COUNT        1
static bool g_continueRunning = true;
static size_t g_message_count_send_confirmations = 0;

static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    (void)user_context;
    const char* messageId;
    const char* correlationId;

    // Message properties
    if ((messageId = IoTHubMessage_GetMessageId(message)) == NULL)
    {
        messageId = "<unavailable>";
    }

    if ((correlationId = IoTHubMessage_GetCorrelationId(message)) == NULL)
    {
        correlationId = "<unavailable>";
    }

    IOTHUBMESSAGE_CONTENT_TYPE content_type = IoTHubMessage_GetContentType(message);
    if (content_type == IOTHUBMESSAGE_BYTEARRAY)
    {
        const unsigned char* buff_msg;
        size_t buff_len;

        if (IoTHubMessage_GetByteArray(message, &buff_msg, &buff_len) != IOTHUB_MESSAGE_OK)
        {
            (void)printf("Failure retrieving byte array message\r\n");
        }
        else
        {
            (void)printf("Received Binary message\r\nMessage ID: %s\r\n Correlation ID: %s\r\n Data: <<<%.*s>>> & Size=%d\r\n", messageId, correlationId, (int)buff_len, buff_msg, (int)buff_len);
        }
    }
    else
    {
        const char* string_msg = IoTHubMessage_GetString(message);
        if (string_msg == NULL)
        {
            (void)printf("Failure retrieving byte array message\r\n");
        }
        else
        {
            (void)printf("Received String Message\r\nMessage ID: %s\r\n Correlation ID: %s\r\n Data: <<<%s>>>\r\n", messageId, correlationId, string_msg);
        }
    }
    return IOTHUBMESSAGE_ACCEPTED;
}

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    // When a message is sent this callback will get envoked
    g_message_count_send_confirmations++;
    (void)printf("Confirmation callback received for message %zu with result %s\r\n", g_message_count_send_confirmations, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    (void)user_context;
    // This sample DOES NOT take into consideration network outages.
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
    {
        (void)printf("The device client is connected to iothub\r\n");
    }
    else
    {
        (void)printf("The device client has been disconnected\r\n");
    }
}

static void device_twin_callback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payload, size_t size, void* user_ctx)
{
    (void)update_state;
    (void)size;
    (void)user_ctx;

    (void)printf("Twin Payload %.*s\r\n", (int)size, payload);
}

static int device_method_callback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* user_ctx)
{
    int result;
    (void)user_ctx;

    (void)printf("Method Name: %s - Payload %.*s\r\n", method_name, (int)size, payload);

    const char deviceMethodResponse[] = "{ \"Response\": \"1HGCM82633A004352\" }";
    *response_size = sizeof(deviceMethodResponse) - 1;
    *response = malloc(*response_size);
    (void)memcpy(*response, deviceMethodResponse, *response_size);
    result = 200;

    return result;
}

static void iothub_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)user_context;
    (void)result;
    //(void)printf("iothub_connection_status result: %s reason: %s\r\n", ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, result), ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));

    if (reason == IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED)
    {
        (void)printf("\"maxMemory\" : %zu\r\n\"currMemory\" : %zu\r\n\"numAlloc\" : %zu\r\n", gballoc_getMaximumMemoryUsed(), gballoc_getCurrentMemoryUsed(), gballoc_getAllocationCount());
    }
}

int main(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_MESSAGE_HANDLE message_handle;
    size_t messages_sent = 0;
    const char* telemetry_msg = "test_message";

    // Select the Protocol to use with the connection
#ifdef USE_MQTT
    #ifdef USE_WEB_SOCKETS
        protocol = MQTT_WebSocket_Protocol;
    #else
        protocol = MQTT_Protocol;
    #endif
#elif USE_AMQP
    #ifdef USE_WEB_SOCKETS
        protocol = AMQP_Protocol_over_WebSocketsTls;
    #else
        protocol = AMQP_Protocol;
    #endif
#else
    // USE_HTTP
    protocol = HTTP_Protocol;
#endif

    IOTHUB_DEVICE_CLIENT_LL_HANDLE dev_ll_handle;

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();

    (void)printf("Creating IoTHub handle\r\n");
    // Create the iothub handle here
    dev_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
    if (dev_ll_handle == NULL)
    {
        (void)printf("Failure creating iothub handle");
    }
    else
    {
        // Set any option that are neccessary.
        // For available options please see the iothub_sdk_options.md documentation
        bool traceOn = true;
        IoTHubDeviceClient_LL_SetOption(dev_ll_handle, OPTION_LOG_TRACE, &traceOn);
        // Setting the Trusted Certificate.  This is only necessary on system with without
        // built in certificate stores.
        IoTHubDeviceClient_LL_SetOption(dev_ll_handle, OPTION_TRUSTED_CERT, certificates);

        (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(dev_ll_handle, iothub_connection_status, NULL);
        // Setting message callback to get C2D messages
        (void)IoTHubDeviceClient_LL_SetMessageCallback(dev_ll_handle, receive_msg_callback, NULL);
        // Setting connection status callback to get indication of connection to iothub
        (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(dev_ll_handle, connection_status_callback, NULL);

        (void)IoTHubDeviceClient_LL_SetDeviceTwinCallback(dev_ll_handle, device_twin_callback, NULL);
        (void)IoTHubDeviceClient_LL_SetDeviceMethodCallback(dev_ll_handle, device_method_callback, NULL);

        gballoc_init();
        do
        {
            if (messages_sent < MESSAGE_COUNT)
            {
                // Construct the iothub message from a string or a byte array
                message_handle = IoTHubMessage_CreateFromString(telemetry_msg);
                //message_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText)));

                // Set Message property
                (void)IoTHubMessage_SetMessageId(message_handle, "MSG_ID");
                (void)IoTHubMessage_SetCorrelationId(message_handle, "CORE_ID");
                (void)IoTHubMessage_SetContentTypeSystemProperty(message_handle, "application%2Fjson");
                (void)IoTHubMessage_SetContentEncodingSystemProperty(message_handle, "utf-8");

                // Add custom properties to message
                IoTHubMessage_SetProperty(message_handle, "property_key", "property_value");

                (void)printf("Sending message %d to IoTHub\r\n", (int)(messages_sent + 1));
                IoTHubDeviceClient_LL_SendEventAsync(dev_ll_handle, message_handle, send_confirm_callback, NULL);

                // The message is copied to the sdk so the we can destroy it
                IoTHubMessage_Destroy(message_handle);

                messages_sent++;
            }
            else if (g_message_count_send_confirmations >= MESSAGE_COUNT)
            {
                // After all messages are all received stop running
                g_continueRunning = false;
            }

            IoTHubDeviceClient_LL_DoWork(dev_ll_handle);
            ThreadAPI_Sleep(100);
        } while (g_continueRunning);

        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(dev_ll_handle);
    }
    // Free all the sdk subsystem
    IoTHub_Deinit();

    gballoc_deinit();

    return 0;
}
