// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "mem_reporter.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/gbnetwork.h"
#include "azure_c_shared_utility/strings.h"

#define USE_MSG_BYTE_ARRAY  1
#define MESSAGES_TO_USE     1
#define MAX_DATA_LEN        128

#ifdef DONT_USE_UPLOADTOBLOB
#define UPLOAD_TO_BLOB_IN_USE   0
#else
#define UPLOAD_TO_BLOB_IN_USE   1
#endif

static const char* const UNKNOWN_TYPE = "unknown";
static const char* const NETWORK_ANALYSIS_JSON_FMT = "{ \"sdkAnalysis\": { \"opType\": \"%s\", \"OS\": \"%s\", \"version\": \"%s\", \"transport\" : \"%s\", \"msgCount\" : %d, \"bytesOnWire\" : %zu, \"numOfSends\" : %zu } }";
static const char* const BINARY_SIZE_JSON_FMT = "{ \"sdkAnalysis\": { \"opType\": \"%s\", \"layer\": \"%s\", \"feature\": \"%s\", \"OS\": \"%s\", \"version\": \"%s\", \"transport\" : \"%s\", \"binarySize\" : \"%ld\"} }";
static const char* const HEAP_ANALYSIS_JSON_FMT = "{ \"sdkAnalysis\": { \"opType\": \"%s\", \"layer\": \"%s\", \"OS\": \"%s\", \"version\": \"%s\", \"transport\" : \"%s\", \"msgCount\" : %d, \"maxMemory\" : %zu, \"currMemory\" : %zu, \"numAlloc\" : %zu } }";

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
        case FEATURE_TELEMETRY_LL:
        case FEATURE_C2D_LL:
        case FEATURE_METHODS_LL:
        case FEATURE_TWIN_LL:
        case FEATURE_PROVISIONING_LL:
            result = "lower layer";
            break;

        case FEATURE_TELEMETRY_UL:
        case FEATURE_C2D_UL:
        case FEATURE_METHODS_UL:
        case FEATURE_TWIN_UL:
        case FEATURE_PROVISIONING_UL:
            result = "upper layer";
            break;
        default:
            result = UNKNOWN_TYPE;
            break;
    }
    return result;
}

static const char* get_feature_type(FEATURE_TYPE type)
{
    const char* result;
    switch (type)
    {
    case FEATURE_TELEMETRY_LL:
    case FEATURE_TELEMETRY_UL:
        result = "d2c";
        break;
    case FEATURE_C2D_LL:
    case FEATURE_C2D_UL:
        result = "c2d";
        break;
    case FEATURE_METHODS_LL:
    case FEATURE_METHODS_UL:
        result = "method";
        break;
    case FEATURE_TWIN_LL:
    case FEATURE_TWIN_UL:
        result = "twin";
        break;
    case FEATURE_PROVISIONING_LL:
    case FEATURE_PROVISIONING_UL:
        result = "provisioning";
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
        case OPERATION_MEMORY:
            result = "memory";
            break;
        case OPERATION_NETWORK:
            result = "network";
            break;
        case OPERATION_BINARY_SIZE:
            result = "binSize";
            break;
        default:
            result = UNKNOWN_TYPE;
            break;
    }
    return result;
}

void report_binary_sizes(const BINARY_INFO* bin_info)
{
    STRING_HANDLE binary_data = STRING_construct_sprintf(BINARY_SIZE_JSON_FMT, get_operation_type(bin_info->operation_type), get_feature_type(bin_info->feature_type), get_layer_type(bin_info->operation_type), OS_NAME, bin_info->iothub_version, get_protocol_name(bin_info->iothub_protocol), bin_info->binary_size);
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
    STRING_HANDLE analysis_data = STRING_construct_sprintf(HEAP_ANALYSIS_JSON_FMT, get_operation_type(iot_mem_info->operation_type), get_layer_type(iot_mem_info->operation_type), OS_NAME, iot_mem_info->iothub_version, get_protocol_name(iot_mem_info->iothub_protocol), iot_mem_info->msg_sent, gballoc_getMaximumMemoryUsed(), gballoc_getCurrentMemoryUsed(), gballoc_getAllocationCount());
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

void record_network_usage(const MEM_ANALYSIS_INFO* iot_mem_info)
{
    STRING_HANDLE network_data = STRING_construct_sprintf(NETWORK_ANALYSIS_JSON_FMT, get_operation_type(iot_mem_info->operation_type), OS_NAME, iot_mem_info->iothub_version, get_protocol_name(iot_mem_info->iothub_protocol), iot_mem_info->msg_sent, gbnetwork_getBytesSent(), gbnetwork_getNumSends());
    if (network_data == NULL)
    {
        (void)printf("ERROR: Failed to allocate networking json\r\n");
    }
    else
    {
        (void)printf("%s\r\n", STRING_c_str(network_data));
        ThreadAPI_Sleep(10);
        upload_to_azure(STRING_c_str(network_data));
        STRING_delete(network_data);
    }

}
