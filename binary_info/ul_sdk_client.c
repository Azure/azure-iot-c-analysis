// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "iothub_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
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
static const char* connectionString = "HostName=<hostname>;DeviceId=<deviceId>;SharedAccessKey=<shared_access_key>";

#define MESSAGE_COUNT        1
static bool g_continueRunning = true;
static size_t g_message_count_send_confirmations = 0;

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    // When a message is sent this callback will get envoked
    g_message_count_send_confirmations++;
    (void)printf("Confirmation callback received for message %zu with result %s\r\n", g_message_count_send_confirmations, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
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

    IOTHUB_CLIENT_HANDLE iothub_handle;

    // Used to initialize IoTHub SDK subsystem
    (void)platform_init();

    (void)printf("Creating IoTHub handle\r\n");
    // Create the iothub handle here
    iothub_handle = IoTHubClient_CreateFromConnectionString(connectionString, protocol);

    // Set any option that are neccessary.
    // For available options please see the iothub_sdk_options.md documentation
    bool traceOn = true;
    IoTHubClient_SetOption(iothub_handle, OPTION_LOG_TRACE, &traceOn);
    // Setting the Trusted Certificate.  This is only necessary on system with without
    // built in certificate stores.
    IoTHubClient_SetOption(iothub_handle, OPTION_TRUSTED_CERT, certificates);

    (void)IoTHubClient_SetConnectionStatusCallback(iothub_handle, iothub_connection_status, NULL);

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
            MAP_HANDLE propMap = IoTHubMessage_Properties(message_handle);
            Map_AddOrUpdate(propMap, "property_key", "property_value");

            (void)printf("Sending message %d to IoTHub\r\n", (int)(messages_sent + 1));
            IoTHubClient_SendEventAsync(iothub_handle, message_handle, send_confirm_callback, NULL);

            // The message is copied to the sdk so the we can destroy it
            IoTHubMessage_Destroy(message_handle);

            messages_sent++;
        }
        else if (g_message_count_send_confirmations >= MESSAGE_COUNT)
        {
            // After all messages are all received stop running
            g_continueRunning = false;
        }

        ThreadAPI_Sleep(100);
    } while (g_continueRunning);

    // Clean up the iothub sdk handle
    IoTHubClient_Destroy(iothub_handle);

    // Free all the sdk subsystem
    platform_deinit();

    gballoc_deinit();

    return 0;
}
