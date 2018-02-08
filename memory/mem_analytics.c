// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#ifdef USE_TELEMETRY
    #include "sdk_mem_analytics.h"
#elif USE_C2D
    #include "c2d_mem_analytics.h"
#endif

#include "azure_c_shared_utility/connection_string_parser.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/gballoc.h"

#include "iothub_service_client_auth.h"
#include "iothub_registrymanager.h"

static const char* DEVICE_CONNECTION_STRING_FMT = "HostName=%s;DeviceId=%s;SharedAccessKey=%s";

#define USE_MSG_BYTE_ARRAY  1
#define MESSAGES_TO_USE    1

typedef enum ARGUEMENT_TYPE_TAG
{
    ARGUEMENT_TYPE_UNKNOWN,
    ARGUEMENT_TYPE_CONNECTION_STRING,
    ARGUEMENT_TYPE_DEVICE_ID,
    ARGUEMENT_TYPE_DEVICE_KEY
} ARGUEMENT_TYPE;

typedef struct MEM_ANALYTIC_INFO_TAG
{
    int create_device;
    const char* connection_string;
    IOTHUB_DEVICE device_info;
} MEM_ANALYTIC_INFO;

static int initialize_sdk()
{
    int result;
    if (platform_init() != 0)
    {
        (void)printf("platform_init failed\r\n");
        result = __LINE__;
    }
    else if (gballoc_init() != 0)
    {
        (void)printf("gballoc_init failed\r\n");
        result = __LINE__;
    }
    else
    {
        // Get SDK
        result = 0;
    }
    return result;
}

static int create_device(MEM_ANALYTIC_INFO* mem_info)
{
    int result;

    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE svc_client_handle = IoTHubServiceClientAuth_CreateFromConnectionString(mem_info->connection_string);
    if (svc_client_handle == NULL)
    {
        (void)printf("Failed creating service client handle\r\n");
        result = __LINE__;
    }
    else
    {
        IOTHUB_REGISTRYMANAGER_HANDLE reg_mgr_handle = IoTHubRegistryManager_Create(svc_client_handle);
        if (reg_mgr_handle == NULL)
        {
            (void)printf("Failed creating device registry manager\r\n");
            result = __LINE__;
        }
        else
        {
            IOTHUB_REGISTRY_DEVICE_CREATE register_device;
            memset(&register_device, 0, sizeof(register_device));

            register_device.deviceId = "mem_analytics_device";
            register_device.primaryKey = "";
            register_device.secondaryKey = "";
            register_device.authMethod = IOTHUB_REGISTRYMANAGER_AUTH_SPK;
            IOTHUB_REGISTRYMANAGER_RESULT create_result = IoTHubRegistryManager_CreateDevice(reg_mgr_handle, &register_device, &mem_info->device_info);
            if (create_result != IOTHUB_REGISTRYMANAGER_OK && create_result != IOTHUB_REGISTRYMANAGER_DEVICE_EXIST)
            {
                (void)printf("Failed creating device\r\n");
                result = __LINE__;
            }
            else
            {
                mem_info->create_device = 1;
                result = 0;
            }
            IoTHubRegistryManager_Destroy(reg_mgr_handle);
        }
        IoTHubServiceClientAuth_Destroy(svc_client_handle);
    }
    return result;
}

static void remove_device(MEM_ANALYTIC_INFO* mem_info)
{
    IOTHUB_SERVICE_CLIENT_AUTH_HANDLE svc_client_handle = IoTHubServiceClientAuth_CreateFromConnectionString(mem_info->connection_string);
    if (svc_client_handle == NULL)
    {
        (void)printf("Failed creating service client handle for delete\r\n");
    }
    else
    {
        IOTHUB_REGISTRYMANAGER_HANDLE reg_mgr_handle = IoTHubRegistryManager_Create(svc_client_handle);
        if (reg_mgr_handle == NULL)
        {
            (void)printf("Failed creating device registry manager for delete\r\n");
        }
        else
        {
            if (IoTHubRegistryManager_DeleteDevice(reg_mgr_handle, mem_info->device_info.deviceId) != IOTHUB_REGISTRYMANAGER_OK)
            {
                (void)printf("Failed removing device\r\n");
            }
            IoTHubRegistryManager_Destroy(reg_mgr_handle);
        }
        IoTHubServiceClientAuth_Destroy(svc_client_handle);
    }
}

static char* construct_dev_conn_string(MEM_ANALYTIC_INFO* mem_info)
{
    char* result;

    MAP_HANDLE parse_handle = connectionstringparser_parse_from_char(mem_info->connection_string);
    if (parse_handle == NULL)
    {
        (void)printf("Failure parsing connection string\r\n");
        result = NULL;
    }
    else
    {
        const char* hostname = Map_GetValueFromKey(parse_handle, "HostName");
        if (hostname == NULL)
        {
            (void)printf("Failure parsing connection string\r\n");
            result = NULL;
        }
        else
        {
            size_t alloc_len = strlen(hostname)+strlen(mem_info->device_info.deviceId)+strlen(mem_info->device_info.primaryKey)+strlen(DEVICE_CONNECTION_STRING_FMT);
            if ((result = malloc(alloc_len+1)) == NULL)
            {
                (void)printf("Failure allocating device connection string\r\n");
                result = NULL;
            }
            else if (sprintf(result, DEVICE_CONNECTION_STRING_FMT, hostname, mem_info->device_info.deviceId, mem_info->device_info.primaryKey) == 0)
            {
                (void)printf("Failure constructing device connection string\r\n");
                free(result);
                result = NULL;
            }
        }
        Map_Destroy(parse_handle);
    }
    return result;
}

static int parse_command_line(int argc, char* argv[], MEM_ANALYTIC_INFO* mem_info)
{
    int result = 0;
    ARGUEMENT_TYPE argument_type = ARGUEMENT_TYPE_UNKNOWN;

    for (int index = 0; index < argc; index++)
    {
        if (argument_type == ARGUEMENT_TYPE_UNKNOWN)
        {
            if (argv[index][0] == '-' && (argv[index][1] == 'c' || argv[index][1] == 'C'))
            {
                argument_type = ARGUEMENT_TYPE_CONNECTION_STRING;
            }
            else if (argv[index][0] == '-' && (argv[index][1] == 'd' || argv[index][1] == 'D'))
            {
                argument_type = ARGUEMENT_TYPE_DEVICE_ID;
            }
            else if (argv[index][0] == '-' && (argv[index][1] == 'k' || argv[index][1] == 'K'))
            {
                argument_type = ARGUEMENT_TYPE_DEVICE_KEY;
            }
        }
        else
        {
            switch (argument_type)
            {
                case ARGUEMENT_TYPE_CONNECTION_STRING:
                    mem_info->connection_string = argv[index];
                    break;
                case ARGUEMENT_TYPE_DEVICE_ID:
                    mem_info->device_info.deviceId = argv[index];
                    break;
                case ARGUEMENT_TYPE_DEVICE_KEY:
                    mem_info->device_info.primaryKey = argv[index];
                    break;
                case ARGUEMENT_TYPE_UNKNOWN:
                default:
                    result = __LINE__;
                    break;
            }
            argument_type = ARGUEMENT_TYPE_UNKNOWN;
        }
    }

    if (mem_info->device_info.deviceId == NULL)
    {
        result = create_device(mem_info);
    }
    return result;
}

int main(int argc, char* argv[])
{
    int result;
    MEM_ANALYTIC_INFO mem_info;
    char* device_conn_string;
    memset(&mem_info, 0, sizeof(mem_info));

    if (parse_command_line(argc, argv, &mem_info) != 0)
    {
        (void)printf("Failure parsing command line\r\n");
        result = __LINE__;
    }
    else if ((device_conn_string = construct_dev_conn_string(&mem_info)) == NULL)
    {
        (void)printf("failed construct dev\r\n");
        result = __LINE__;
    }
    else if (initialize_sdk() != 0)
    {
        (void)printf("initializing SDK failed\r\n");
        free(device_conn_string);
        result = __LINE__;
    }
    else
    {
        result = 0;
        // AMQP Sending
        initiate_lower_level_operation(device_conn_string, PROTOCOL_AMQP, MESSAGES_TO_USE, USE_MSG_BYTE_ARRAY);
        initiate_lower_level_operation(device_conn_string, PROTOCOL_AMQP_WS, MESSAGES_TO_USE, USE_MSG_BYTE_ARRAY);
        initiate_upper_level_operation(device_conn_string, PROTOCOL_AMQP, MESSAGES_TO_USE, USE_MSG_BYTE_ARRAY);
        initiate_upper_level_operation(device_conn_string, PROTOCOL_AMQP_WS, MESSAGES_TO_USE, USE_MSG_BYTE_ARRAY);

        // HTTP Sending
        initiate_lower_level_operation(device_conn_string, PROTOCOL_HTTP, MESSAGES_TO_USE, USE_MSG_BYTE_ARRAY);
        initiate_upper_level_operation(device_conn_string, PROTOCOL_HTTP, MESSAGES_TO_USE, USE_MSG_BYTE_ARRAY);

        // Send MQTT messages
        initiate_lower_level_operation(device_conn_string, PROTOCOL_MQTT, MESSAGES_TO_USE, USE_MSG_BYTE_ARRAY);
        initiate_lower_level_operation(device_conn_string, PROTOCOL_MQTT_WS, MESSAGES_TO_USE, USE_MSG_BYTE_ARRAY);
        initiate_upper_level_operation(device_conn_string, PROTOCOL_MQTT, MESSAGES_TO_USE, USE_MSG_BYTE_ARRAY);
        initiate_upper_level_operation(device_conn_string, PROTOCOL_MQTT_WS, MESSAGES_TO_USE, USE_MSG_BYTE_ARRAY);

        if (mem_info.create_device != 0)
        {
            remove_device(&mem_info);
        }

        gballoc_deinit();
        platform_deinit();

        free((char*)mem_info.device_info.deviceId);
        free((char*)mem_info.device_info.primaryKey);
        free((char*)mem_info.device_info.secondaryKey);
        free((char*)mem_info.device_info.generationId);
        free((char*)mem_info.device_info.eTag);
        free((char*)mem_info.device_info.connectionStateUpdatedTime);
        free((char*)mem_info.device_info.statusReason);
        free((char*)mem_info.device_info.statusUpdatedTime);
        free((char*)mem_info.device_info.lastActivityTime);
        free((char*)mem_info.device_info.configuration);
        free((char*)mem_info.device_info.deviceProperties);
        free((char*)mem_info.device_info.serviceProperties);

        free(device_conn_string);
    }

#ifdef _DEBUG
    (void)printf("Press any key to continue:");
    getchar();
#endif // _DEBUG

    return result;
}
