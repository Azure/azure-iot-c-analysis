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
#include "iothub_transport_ll.h"
#include "iothub_client_ll.h"

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
    int result;
    const TRANSPORT_PROVIDER* transport_prov;
    const IOTHUBTRANSPORT_CONFIG config;

    memset(&config, 0, sizeof(IOTHUBTRANSPORT_CONFIG));

#ifdef USE_AMQP
    #ifdef USE_WEB_SOCKETS
        transport_prov = AMQP_Protocol_over_WebSocketsTls();
    #else
        transport_prov = AMQP_Protocol();
    #endif
#elif USE_MQTT
    #ifdef USE_WEB_SOCKETS
        transport_prov = MQTT_WebSocket_Protocol;
    #else
        transport_prov = MQTT_Protocol();
    #endif
#else
    // USE_HTTP
    transport_prov = HTTP_Protocol();
#endif

    TRANSPORT_LL_HANDLE trans_handle = transport_prov->IoTHubTransport_Create(&config);
    if (trans_handle == NULL)
    {
        result = 0;
    }
    else
    {

    }
}
