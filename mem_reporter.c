// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "mem_reporter.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/gballoc.h"

#define USE_MSG_BYTE_ARRAY  1
#define MESSAGES_TO_USE    1

static const char* UNKNOWN_TYPE = "unknown";

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

static const char* get_operation_type(OPERATION_TYPE type)
{
    const char* result;
    switch (type)
    {
        case OPERATION_TELEMETRY_LL:
            result = "telemetry lower layer";
            break;
        case OPERATION_TELEMETRY_UL:
            result = "telemetry upper layer";
            break;
        case OPERATION_C2D_LL:
            result = "c2d lower layer";
            break;
        case OPERATION_C2D_UL:
            result = "c2d upper layer";
            break;
        case OPERATION_BINARY_SIZE_LL:
            result = "binary size lower layer";
            break;
        case OPERATION_BINARY_SIZE_UL:
            result = "binary size upper layer";
            break;
        case OPERATION_METHODS:
            result = "iothub method";
            break;
        case OPERATION_TWIN:
            result = "iothub twin";
            break;
        case OPERATION_PROVISIONING:
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
    const char* json_fmt = "{ \"sdk_analysis\": { \"operationType\": \"%s\", \"OS\": \"%s\", \"version\": \"%s\", \"Transport\" : \"%s\", \"binary_size\" : \"%ld\"} }";
    (void)printf(json_fmt, get_operation_type(bin_info->operation_type), OS_NAME, bin_info->iothub_version, get_protocol_name(bin_info->iothub_protocol), bin_info->binary_size);
    (void)printf("\r\n");
    ThreadAPI_Sleep(10);
}

void record_memory_usage(const MEM_ANALYSIS_INFO* iot_mem_info)
{
    // Construct json
    const char* json_fmt = "{ \"sdk_analysis\": { \"operationType\": \"%s\", \"OS\": \"%s\", \"version\": \"%s\", \"Transport\" : \"%s\", \"msgCount\" : %d, \"maxMemory\" : %zu, \"currMemory\" : %zu, \"numAlloc\" : %zu } }";
    (void)printf(json_fmt, get_operation_type(iot_mem_info->operation_type), OS_NAME, iot_mem_info->iothub_version, get_protocol_name(iot_mem_info->iothub_protocol), iot_mem_info->msg_sent, gballoc_getMaximumMemoryUsed(), gballoc_getCurrentMemoryUsed(), gballoc_getAllocationCount());
    (void)printf("\r\n");
    ThreadAPI_Sleep(10);
    // Upload this to azure?
}
