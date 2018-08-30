// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "provisioning_mem.h"
#include "mem_reporter.h"

#include "iothub_client.h"
#include "iothub_message.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/gballoc.h"

#include "azure_prov_client/prov_device_client.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"

#include "azure_prov_client/prov_transport_http_client.h"
#include "azure_prov_client/prov_transport_amqp_client.h"
#include "azure_prov_client/prov_transport_amqp_ws_client.h"
#include "azure_prov_client/prov_transport_mqtt_ws_client.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"

#include "../../../certs/certs.h"

#include "../certs/certs.h"

#define WAIT_TIME           2*1000
static const char* const GLOBAL_PROV_URI = "global.azure-devices-provisioning.net";

static const char* id_scope = "";

typedef struct PROV_TEST_INFO_TAG
{
    int connected;
    int registration_complete;
    int error;
} PROV_TEST_INFO;

static PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION initialize(MEM_ANALYSIS_INFO* prov_mem_info, PROTOCOL_TYPE protocol, size_t num_msgs_to_send)
{
    PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION result;
    (void)num_msgs_to_send;

    prov_mem_info->msg_sent = 0;
    prov_mem_info->iothub_version = Prov_Device_GetVersionString();

    prov_mem_info->iothub_protocol = protocol;

    switch (protocol)
    {
        case PROTOCOL_MQTT:
            result = Prov_Device_MQTT_Protocol;
            break;
        case PROTOCOL_MQTT_WS:
            result = Prov_Device_MQTT_WS_Protocol;
            break;
        case PROTOCOL_HTTP:
            result = Prov_Device_HTTP_Protocol;
            break;
        case PROTOCOL_AMQP:
            result = Prov_Device_AMQP_Protocol;
            break;
        case PROTOCOL_AMQP_WS:
            result = Prov_Device_AMQP_WS_Protocol;
            break;
        default:
            result = NULL;
            break;
    }
    return result;
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    PROV_TEST_INFO* prov_info = (PROV_TEST_INFO*)user_context;
    if (register_result == PROV_DEVICE_RESULT_OK)
    {
        prov_info->registration_complete = 1;
    }
    else
    {
        prov_info->error = 1;
    }
}

int initiate_lower_level_operation(const CONNECTION_INFO* conn_info, PROTOCOL_TYPE protocol, size_t num_msgs_to_send, bool use_byte_array_msg)
{
    int result;
    SECURE_DEVICE_TYPE hsm_type;
    //hsm_type = SECURE_DEVICE_TYPE_TPM;
    hsm_type = SECURE_DEVICE_TYPE_X509;

    if (prov_dev_security_init(hsm_type) != 0)
    {
        (void)printf("prov_dev_security_init failed\r\n");
        result = __LINE__;
    }
    else
    {
        PROV_TEST_INFO prov_info;
        PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION prov_transport;
        PROV_DEVICE_LL_HANDLE prov_device_handle;
        MEM_ANALYSIS_INFO prov_mem_info;
        memset(&prov_mem_info, 0, sizeof(MEM_ANALYSIS_INFO));
        memset(&prov_info, 0, sizeof(PROV_TEST_INFO));

        prov_mem_info.operation_type = OPERATION_MEMORY;
        prov_mem_info.feature_type = FEATURE_PROVISIONING_LL;

        gballoc_resetMetrics();

        if ((prov_transport = initialize(&prov_mem_info, protocol, num_msgs_to_send)) == NULL)
        {
            (void)printf("failed initializing operation\r\n");
            result = __LINE__;
        }
        else if ((prov_device_handle = Prov_Device_LL_Create(GLOBAL_PROV_URI, conn_info->scope_id, prov_transport)) == NULL)
        {
            (void)printf("failed calling Prov_Device_Create\r\n");
            result = __LINE__;
        }
        else
        {
            Prov_Device_LL_SetOption(prov_device_handle, OPTION_TRUSTED_CERT, certificates);

            if (Prov_Device_LL_Register_Device(prov_device_handle, register_device_callback, &prov_info, NULL, NULL) != PROV_DEVICE_RESULT_OK)
            {
                (void)printf("failed calling Prov_Device_LL_Register_Device\r\n");
                result = __LINE__;
            }
            else
            {
                result = 0;
                do
                {
                    Prov_Device_LL_DoWork(prov_device_handle);
                    ThreadAPI_Sleep(WAIT_TIME);
                } while (prov_info.registration_complete == 0);
            }
            Prov_Device_LL_Destroy(prov_device_handle);
        }
        prov_dev_security_deinit();

        report_memory_usage(&prov_mem_info);
    }
    return result;
}

int initiate_upper_level_operation(const CONNECTION_INFO* conn_info, PROTOCOL_TYPE protocol, size_t num_msgs_to_send, bool use_byte_array_msg)
{
    int result;
    SECURE_DEVICE_TYPE hsm_type;
    //hsm_type = SECURE_DEVICE_TYPE_TPM;
    hsm_type = SECURE_DEVICE_TYPE_X509;

    if (prov_dev_security_init(hsm_type) != 0)
    {
        (void)printf("prov_dev_security_init failed\r\n");
        result = __LINE__;
    }
    else
    {
        PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION prov_transport;
        PROV_TEST_INFO prov_info;
        PROV_DEVICE_HANDLE prov_device_handle;
        MEM_ANALYSIS_INFO prov_mem_info;
        memset(&prov_mem_info, 0, sizeof(MEM_ANALYSIS_INFO));
        memset(&prov_info, 0, sizeof(PROV_TEST_INFO));

        prov_mem_info.operation_type = OPERATION_MEMORY;
        prov_mem_info.feature_type = FEATURE_PROVISIONING_UL;

        gballoc_resetMetrics();

        if ((prov_transport = initialize(&prov_mem_info, protocol, num_msgs_to_send)) == NULL)
        {
            (void)printf("failed initializing operation\r\n");
            result = __LINE__;
        }
        else if ((prov_device_handle = Prov_Device_Create(GLOBAL_PROV_URI, conn_info->scope_id, prov_transport)) == NULL)
        {
            (void)printf("failed calling Prov_Device_Create\r\n");
            result = __LINE__;
        }
        else
        {
            Prov_Device_SetOption(prov_device_handle, OPTION_TRUSTED_CERT, certificates);

            if (Prov_Device_Register_Device(prov_device_handle, register_device_callback, &prov_info, NULL, NULL) != PROV_DEVICE_RESULT_OK)
            {
                (void)printf("failed calling Prov_Device_Register_Device\r\n");
                result = __LINE__;
            }
            else
            {
                result = 0;
                do
                {
                    ThreadAPI_Sleep(WAIT_TIME);
                } while (prov_info.registration_complete == 0);
            }
            Prov_Device_Destroy(prov_device_handle);
        }
        prov_dev_security_deinit();
        report_memory_usage(&prov_mem_info);
    }
    return result;
}
