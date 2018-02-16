// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "mem_reporter.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/strings.h"

#define USE_MSG_BYTE_ARRAY  1
#define MESSAGES_TO_USE     1
#define MAX_DATA_LEN        128

static const char* UNKNOWN_TYPE = "unknown";

static void upload_to_azure(const char* data)
{
    (void)data;
    // TODO upload to azure iot
}

static const char* get_protocol_name(PROTOCOL_TYPE protocol)
{
    const char* result;
    switch (protocol)
    {
        case PROTOCOL_MQTT:
            result = MQTT_PROTOCOL_NAME;
            break;
        case PROTOCOL_MQTT_WS:
            result = MQTT_WS_PROTOCOL_NAME;
            break;
        case PROTOCOL_HTTP:
            result = HTTP_PROTOCOL_NAME;
            break;
        case PROTOCOL_AMQP:
            result = AMQP_PROTOCOL_NAME;
            break;
        case PROTOCOL_AMQP_WS:
            result = AMQP_WS_PROTOCOL_NAME;
            break;
        default:
        case PROTOCOL_UNKNOWN:
            result = UNKNOWN_TYPE;
            break;
    }
    return result;
}

static const char* get_layer_type(OPERATION_TYPE type)
{
    const char* result;
    switch (type)
    {
        case OPERATION_TELEMETRY_LL:
        case OPERATION_C2D_LL:
        case OPERATION_BINARY_SIZE_LL:
        case OPERATION_METHODS_LL:
        case OPERATION_TWIN_LL:
        case OPERATION_PROVISIONING_LL:
            result = "lower_layer";
            break;

        case OPERATION_TELEMETRY_UL:
        case OPERATION_C2D_UL:
        case OPERATION_BINARY_SIZE_UL:
        case OPERATION_METHODS_UL:
        case OPERATION_TWIN_UL:
        case OPERATION_PROVISIONING_UL:
            result = "upper_layer";
            break;
        default:
            result = UNKNOWN_TYPE;
            break;
    }
    return result;
}

static const char* get_operation_type(OPERATION_TYPE type)
{
    const char* result;
    switch (type)
    {
        case OPERATION_TELEMETRY_LL:
        case OPERATION_TELEMETRY_UL:
            result = "telemetry";
            break;
        case OPERATION_C2D_LL:
        case OPERATION_C2D_UL:
            result = "cloud 2 device";
            break;
        case OPERATION_BINARY_SIZE_LL:
        case OPERATION_BINARY_SIZE_UL:
            result = "binary size";
            break;
        case OPERATION_METHODS_LL:
        case OPERATION_METHODS_UL:
            result = "iothub method";
            break;
        case OPERATION_TWIN_LL:
        case OPERATION_TWIN_UL:
            result = "iothub twin";
            break;
        case OPERATION_PROVISIONING_LL:
        case OPERATION_PROVISIONING_UL:
            result = "iothub provisioning";
            break;
        default:
            result = UNKNOWN_TYPE;
            break;
    }
    return result;
}

void report_binary_sizes(const BINARY_INFO* bin_info)
{
    const char* const BINARY_SIZE_JSON_FMT = "{ \"sdk_analysis\": { \"operationType\": \"%s\", \"layer_type\": \"%s\", \"OS\": \"%s\", \"version\": \"%s\", \"Transport\" : \"%s\", \"binary_size\" : \"%ld\"} }";
    STRING_HANDLE binary_data = STRING_construct_sprintf(BINARY_SIZE_JSON_FMT, get_operation_type(bin_info->operation_type), get_layer_type(bin_info->operation_type), OS_NAME, bin_info->iothub_version, get_protocol_name(bin_info->iothub_protocol), bin_info->binary_size);
    if (binary_data == NULL)
    {
        (void)printf("ERROR: Failed to allocate binary json\r\n");
    }
    else
    {
        (void)printf("%s\r\n", STRING_c_str(binary_data));
        ThreadAPI_Sleep(10);
        upload_to_azure(STRING_c_str(binary_data));
        STRING_delete(binary_data);
    }
}

void record_memory_usage(const MEM_ANALYSIS_INFO* iot_mem_info)
{
    // Construct json
    const char* json_fmt = "{ \"sdk_analysis\": { \"operationType\": \"%s\", \"layer_type\": \"%s\", \"OS\": \"%s\", \"version\": \"%s\", \"Transport\" : \"%s\", \"msgCount\" : %d, \"maxMemory\" : %zu, \"currMemory\" : %zu, \"numAlloc\" : %zu } }";
    STRING_HANDLE analysis_data = STRING_construct_sprintf(json_fmt, get_operation_type(iot_mem_info->operation_type), get_layer_type(iot_mem_info->operation_type), OS_NAME, iot_mem_info->iothub_version, get_protocol_name(iot_mem_info->iothub_protocol), iot_mem_info->msg_sent, gballoc_getMaximumMemoryUsed(), gballoc_getCurrentMemoryUsed(), gballoc_getAllocationCount());
    if (analysis_data == NULL)
    {
        (void)printf("ERROR: Failed to allocate memory json\r\n");
    }
    else
    {
        (void)printf("%s\r\n", STRING_c_str(analysis_data));
        ThreadAPI_Sleep(10);
        upload_to_azure(STRING_c_str(analysis_data));
        STRING_delete(analysis_data);
    }
}
