// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/gballoc.h"

#include "azure_c_shared_utility/shared_util_options.h"

#ifdef USE_MQTT
    #ifdef USE_WEB_SOCKETS
        #include "azure_prov_client/prov_transport_mqtt_ws_client.h"
    #else
        #include "azure_prov_client/prov_transport_mqtt_client.h"
    #endif
#elif USE_AMQP
    #ifdef USE_WEB_SOCKETS
        #include "azure_prov_client/prov_transport_amqp_ws_client.h"
    #else
        #include "azure_prov_client/prov_transport_amqp_client.h"
    #endif
#else // USE_HTTP
    #include "azure_prov_client/prov_transport_http_client.h"
#endif

#include "certs.h"

/* String containing Hostname, Device Id & Device Key in the format:                         */
/* Paste in the your iothub connection string  */
static const char* const g_global_prov_uri = "global_uri";
static const char* id_scope = "scope_id";

static bool g_trace_on = true;

#ifdef USE_OPENSSL
static bool g_using_cert = true;
#else
static bool g_using_cert = false;
#endif // USE_OPENSSL

#define PROXY_PORT                  8888
#define MESSAGES_TO_SEND            2
#define TIME_BETWEEN_MESSAGES       2

typedef struct CLIENT_SAMPLE_INFO_TAG
{
    unsigned int sleep_time;
    char* iothub_uri;
    char* access_key_name;
    char* device_key;
    char* device_id;
    int registration_complete;
} CLIENT_SAMPLE_INFO;

typedef struct IOTHUB_CLIENT_SAMPLE_INFO_TAG
{
    int connected;
    int stop_running;
} IOTHUB_CLIENT_SAMPLE_INFO;

static void registation_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
    if (user_context == NULL)
    {
        printf("user_context is NULL\r\n");
    }
    else
    {
        if (reg_status == PROV_DEVICE_REG_STATUS_CONNECTED)
        {
        }
        else if (reg_status == PROV_DEVICE_REG_STATUS_REGISTERING)
        {
        }
        else if (reg_status == PROV_DEVICE_REG_STATUS_ASSIGNING)
        {
        }
    }
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    if (user_context == NULL)
    {
        printf("user_context is NULL\r\n");
    }
    else
    {
        CLIENT_SAMPLE_INFO* user_ctx = (CLIENT_SAMPLE_INFO*)user_context;
        if (register_result == PROV_DEVICE_RESULT_OK)
        {
            (void)printf("Registration Information received from service: %s!\r\n", iothub_uri);
            (void)mallocAndStrcpy_s(&user_ctx->iothub_uri, iothub_uri);
            (void)mallocAndStrcpy_s(&user_ctx->device_id, device_id);
            user_ctx->registration_complete = 1;
        }
        else
        {
            (void)printf("Failure encountered on registration!\r\n");
            user_ctx->registration_complete = 2;
        }
    }
}

int main()
{
    int result;
    SECURE_DEVICE_TYPE hsm_type;
    //hsm_type = SECURE_DEVICE_TYPE_TPM;
    hsm_type = SECURE_DEVICE_TYPE_X509;

    if (platform_init() != 0)
    {
        (void)printf("platform_init failed\r\n");
        result = __LINE__;
    }
    else if (prov_dev_security_init(hsm_type) != 0)
    {
        (void)printf("prov_dev_security_init failed\r\n");
        result = __LINE__;
    }
    else
    {
        const char* trusted_cert;
        PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION g_prov_transport;
        CLIENT_SAMPLE_INFO user_ctx;

        memset(&user_ctx, 0, sizeof(CLIENT_SAMPLE_INFO));

        if (g_using_cert)
        {
            trusted_cert = certificates;
        }
        else
        {
            trusted_cert = NULL;
        }

        // Protocol to USE - HTTP, AMQP, AMQP_WS, MQTT, MQTT_WS
#ifdef USE_AMQP
#ifdef USE_WEB_SOCKETS
        g_prov_transport = Prov_Device_AMQP_WS_Protocol;
#else
        g_prov_transport = Prov_Device_AMQP_Protocol;
#endif
#elif USE_MQTT
#ifdef USE_WEB_SOCKETS
        g_prov_transport = Prov_Device_MQTT_WS_Protocol;
#else
        g_prov_transport = Prov_Device_MQTT_Protocol;
#endif
#else
        g_prov_transport = Prov_Device_HTTP_Protocol;
#endif


        PROV_DEVICE_LL_HANDLE handle;
        if ((handle = Prov_Device_LL_Create(g_global_prov_uri, id_scope, g_prov_transport)) == NULL)
        {
            (void)printf("failed calling Prov_Device_LL_Create\r\n");
            result = __LINE__;
        }
        else
        {
            Prov_Device_LL_SetOption(handle, "logtrace", &g_trace_on);
            if (trusted_cert != NULL)
            {
                Prov_Device_LL_SetOption(handle, OPTION_TRUSTED_CERT, trusted_cert);
            }
            Prov_Device_LL_SetOption(handle, PROV_REGISTRATION_ID, "manual-reg-id");

            if (Prov_Device_LL_Register_Device(handle, register_device_callback, &user_ctx, registation_status_callback, &user_ctx) != PROV_DEVICE_RESULT_OK)
            {
                (void)printf("failed calling Prov_Device_LL_Register_Device\r\n");
                result = __LINE__;
            }
            else
            {
                do
                {
                    Prov_Device_LL_DoWork(handle);
                    ThreadAPI_Sleep(user_ctx.sleep_time);
                } while (user_ctx.registration_complete == 0);
            }
            Prov_Device_LL_Destroy(handle);
        }
    }
    return result;
}
