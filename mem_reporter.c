// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "mem_reporter.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/gbnetwork.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/agenttime.h"

#include "parson.h"

#define USE_MSG_BYTE_ARRAY  1
#define MESSAGES_TO_USE     1
#define MAX_DATA_LEN        128
#define DATE_TIME_LEN       64

#ifdef DONT_USE_UPLOADTOBLOB
#define UPLOAD_TO_BLOB_IN_USE   0
#else
#define UPLOAD_TO_BLOB_IN_USE   1
#endif

static REPORTER_TYPE g_report_type = REPORTER_TYPE_JSON;

static const char* const UNKNOWN_TYPE = "unknown";
static const char* const SDK_ANALYSIS_NODE = "sdkAnalysis";
static const char* const NETWORK_ANALYSIS_JSON_FMT = "{ \"sdkAnalysis\": { \"dateTime\": \"%s\", \"opType\": \"%s\", \"feature\": \"%s\", \"OS\": \"%s\", \"version\": \"%s\", \"transport\" : \"%s\", \"msgPayload\" : %d, \"sendBytes\" : %" PRIu64 ", \"numSends\" : %ld, \"recvBytes\" : %" PRIu64 ", \"numRecv\" : %ld } }";
static const char* const NETWORK_ANALYSIS_CVS_FMT = "%s, %s, %s, %s, %s, %s, %d, %" PRIu64 ", %ld, %" PRIu64 ", %ld";
static const char* const BINARY_SIZE_JSON_FMT = "{ \"sdkAnalysis\": { \"dateTime\": \"%s\", \"opType\": \"%s\", \"feature\": \"%s\", \"layer\": \"%s\", \"OS\": \"%s\", \"version\": \"%s\", \"transport\" : \"%s\", \"binarySize\" : \"%ld\"} }";
static const char* const BINARY_SIZE_CVS_FMT = "%s, %s, %s, %s, %s, %s, %s, %ld";
static const char* const HEAP_ANALYSIS_JSON_FMT = "{ \"sdkAnalysis\": { \"dateTime\": \"%s\", \"opType\": \"%s\", \"feature\": \"%s\", \"layer\": \"%s\", \"OS\": \"%s\", \"version\": \"%s\", \"transport\" : \"%s\", \"msgCount\" : %d, \"maxMemory\" : %zu, \"currMemory\" : %zu, \"numAlloc\" : %zu } }";
static const char* const HEAP_ANALYSIS_CVS_FMT = "%s, %s, %s, %s, %s, %s, %s, %d, %zu, %zu, %zu";

typedef struct REPORT_INFO_TAG
{
    REPORTER_TYPE type;
    JSON_Value* root_value;
} REPORT_INFO;

static void upload_to_azure(const char* data)
{
    (void)data;
    // TODO upload to azure iot
}

static void get_report_date(char* date, size_t length)
{
    time_t curr_time = get_time(NULL);
    char* report_time = NULL;
    memset(date, 0, length);
    struct tm* tm_val = localtime(&curr_time);
    sprintf(date, "%d-%d-%d", tm_val->tm_mon+1, tm_val->tm_mday, tm_val->tm_year+1900);
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

static const char* get_layer_type(FEATURE_TYPE type)
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
        result = "dps";
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

static const char* get_format_value(OPERATION_TYPE type)
{
    const char* result;
    switch (type)
    {
        case OPERATION_MEMORY:
            if (g_report_type == REPORTER_TYPE_CVS)
            {
                result = HEAP_ANALYSIS_CVS_FMT;
            }
            else
            {
                result = HEAP_ANALYSIS_JSON_FMT;
            }
            break;
        case OPERATION_NETWORK:
            if (g_report_type == REPORTER_TYPE_CVS)
            {
                result = NETWORK_ANALYSIS_CVS_FMT;
            }
            else
            {
                result = NETWORK_ANALYSIS_JSON_FMT;
            }
            break;
        case OPERATION_BINARY_SIZE:
            if (g_report_type == REPORTER_TYPE_CVS)
            {
                result = BINARY_SIZE_CVS_FMT;
            }
            else
            {
                result = BINARY_SIZE_JSON_FMT;
            }
            break;
        default:
            result = UNKNOWN_TYPE;
            break;
    }
    return result;
}

REPORT_HANDLE report_initialize(REPORTER_TYPE type)
{
    REPORT_INFO* result;
    if ((result = (REPORT_INFO*)malloc(sizeof(REPORT_INFO))) == NULL)
    {
        // Report failure
    }
    else
    {
        result->type = type;
    }
    return result;
}

void report_deinitialize(REPORT_HANDLE handle)
{
    // Close the file
    if (handle == NULL)
    {
        free(handle);
    }
}

void report_binary_sizes(REPORT_HANDLE handle, const BINARY_INFO* bin_info)
{
    char date_time[DATE_TIME_LEN];
    get_report_date(date_time, DATE_TIME_LEN);

    const char* string_format = get_format_value(bin_info->operation_type);
    STRING_HANDLE binary_data = STRING_construct_sprintf(string_format, date_time, get_operation_type(bin_info->operation_type), get_feature_type(bin_info->feature_type),
        get_layer_type(bin_info->feature_type), OS_NAME, bin_info->iothub_version, get_protocol_name(bin_info->iothub_protocol), bin_info->binary_size);
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

void report_memory_usage(const MEM_ANALYSIS_INFO* iot_mem_info)
{
    char date_time[DATE_TIME_LEN];
    get_report_date(date_time, DATE_TIME_LEN);

    const char* string_format = get_format_value(iot_mem_info->operation_type);
    STRING_HANDLE analysis_data = STRING_construct_sprintf(string_format, date_time, get_operation_type(iot_mem_info->operation_type), get_feature_type(iot_mem_info->feature_type), get_layer_type(iot_mem_info->feature_type), OS_NAME,
        iot_mem_info->iothub_version, get_protocol_name(iot_mem_info->iothub_protocol), iot_mem_info->msg_sent, gballoc_getMaximumMemoryUsed(), gballoc_getCurrentMemoryUsed(), gballoc_getAllocationCount());
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

void report_network_usage(const MEM_ANALYSIS_INFO* iot_mem_info)
{
    char date_time[DATE_TIME_LEN];
    get_report_date(date_time, DATE_TIME_LEN);

    STRING_HANDLE network_data = STRING_construct_sprintf(NETWORK_ANALYSIS_JSON_FMT, date_time, get_operation_type(iot_mem_info->operation_type), get_feature_type(iot_mem_info->feature_type), OS_NAME, iot_mem_info->iothub_version,
        get_protocol_name(iot_mem_info->iothub_protocol), iot_mem_info->msg_sent, gbnetwork_getBytesSent(), (uint32_t)gbnetwork_getNumSends(), gbnetwork_getBytesRecv(), (uint32_t)gbnetwork_getNumRecv());
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

bool report_write(REPORT_HANDLE handle)
{
    bool result;
    return result;
}
