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
#define FORMAT_MAX_LEN      64

#ifdef DONT_USE_UPLOADTOBLOB
#define UPLOAD_TO_BLOB_IN_USE   0
#else
#define UPLOAD_TO_BLOB_IN_USE   1
#endif

static REPORTER_TYPE g_report_type = REPORTER_TYPE_JSON;

static const char* const UNKNOWN_TYPE = "unknown";
static const char* const NODE_SDK_ANALYSIS = "sdkAnalysis";
static const char* const NODE_BASE_ARRAY = "analysisItem";
static const char* const SDK_ANALYSIS_EMPTY_NODE = "{ \"sdkAnalysis\" : { \"osType\": \"%s\", \"uploadEnabled\": \"%s\", \"logEnabled\": \"%s\", \"analysisItem\" : [] } }";
static const char* const NODE_OPERATING_SYSTEM = "osType";

static const char* const BINARY_SIZE_JSON_FMT = "{ \"type\": \"ROM\", \"dateTime\": \"%s\", \"feature\": \"%s\", \"layer\": \"%s\", \"version\": \"%s\", \"transport\" : \"%s\", \"binarySize\" : \"%s\" }";
static const char* const HEAP_ANALYSIS_JSON_FMT = "{ \"type\": \"RAM\", \"dateTime\": \"%s\", \"feature\": \"%s\", \"layer\": \"%s\", \"version\": \"%s\", \"transport\" : \"%s\", \"msgCount\" : %d, \"maxMemory\" : \"%s\", \"currMemory\" : \"%s\", \"numAlloc\" : \"%s\" } }";
static const char* const NETWORK_ANALYSIS_JSON_FMT = "{ \"type\": \"NETWORK\", \"dateTime\": \"%s\", \"feature\": \"%s\", \"version\": \"%s\", \"transport\" : \"%s\", \"msgPayload\" : %d, \"bytesSent\" : \"%s\", \"numSends\" : \"%s\", \"recvBytes\" : \"%s\", \"numRecv\" : \"%s\" } }";

static const char* const BINARY_SIZE_CVS_FMT = "%s, %s, %s, %s, %s, %s, %s, %ld";
static const char* const HEAP_ANALYSIS_CVS_FMT = "%s, %s, %s, %s, %s, %s, %s, %d, %zu, %zu, %zu";
static const char* const NETWORK_ANALYSIS_CVS_FMT = "%s, %s, %s, %s, %s, %s, %d, %" PRIu64 ", %ld, %" PRIu64 ", %ld";

#ifdef NO_LOGGING
static const char* const LOGGING_INCLUDED = "false";
#else
static const char* const LOGGING_INCLUDED = "true";
#endif
#ifdef DONT_USE_UPLOADTOBLOB
static const char* const UPLOAD_INCLUDED = "false";
#else
static const char* const UPLOAD_INCLUDED = "true";
#endif

typedef struct REPORT_INFO_TAG
{
    REPORTER_TYPE type;
    JSON_Value* root_value;
    JSON_Object* analysis_node;
} REPORT_INFO;

static void format_value(uint64_t value, char formatting[FORMAT_MAX_LEN])
{
    char temp[FORMAT_MAX_LEN];
    sprintf(temp, "%" PRIu64, value);
    memset(formatting, 0, FORMAT_MAX_LEN);

    size_t length = strlen(temp);
    size_t cntr = 0;
    size_t extra_char = length / 3;
    if ((length % 3) == 0)
    {
        extra_char--;
    }

    for (int index = length - 1; index >= 0; index--, cntr++)
    {
        if (cntr >= 3)
        {
            formatting[index + extra_char] = ',';
            extra_char--;
            cntr = 0;
        }
        formatting[index + extra_char] = temp[index];
    }
}

static void format_bytes(long bytes, char formatting[FORMAT_MAX_LEN])
{
    char temp[FORMAT_MAX_LEN];
    sprintf(temp, "%ld", bytes);
    memset(formatting, 0, FORMAT_MAX_LEN);

    size_t length = strlen(temp);
    size_t cntr = 0;
    size_t extra_char = length / 3;
    if ((length % 3) == 0)
    {
        extra_char--;
    }

    for (int index = length-1; index >= 0; index--, cntr++)
    {
        if (cntr >= 3)
        {
            formatting[index + extra_char] = ',';
            extra_char--;
            cntr = 0;
        }
        formatting[index + extra_char] = temp[index];
    }
}

static void add_node_to_json(const char* node_data, const REPORT_INFO* report_info)
{
    JSON_Value* json_value;
    if ((json_value = json_parse_string(node_data)) != NULL)
    {
        JSON_Object* json_object;
        JSON_Array* base_array;
        JSON_Value* json_analysis;

        if ((json_analysis = json_object_get_value(report_info->analysis_node, NODE_SDK_ANALYSIS)) == NULL)
        {
            (void)printf("ERROR: Failed getting node object value\r\n");
        }
        else if ((json_object = json_value_get_object(json_analysis)) == NULL)
        {
            (void)printf("ERROR: Failed getting object value\r\n");
        }
        else if ((base_array = json_object_get_array(json_object, NODE_BASE_ARRAY)) == NULL)
        {
            (void)printf("ERROR: Failed getting object value\r\n");
        }
        else
        {
            if (json_array_append_value(base_array, json_value) != JSONSuccess)
            {
                (void)printf("ERROR: Failed to allocate binary json\r\n");

            }
        }
    }
}

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

static const char* get_format_value(const REPORT_INFO* report_info, OPERATION_TYPE type)
{
    const char* result;
    switch (type)
    {
        case OPERATION_MEMORY:
            if (report_info->type == REPORTER_TYPE_CVS)
            {
                result = HEAP_ANALYSIS_CVS_FMT;
            }
            else
            {
                result = HEAP_ANALYSIS_JSON_FMT;
            }
            break;
        case OPERATION_NETWORK:
            if (report_info->type == REPORTER_TYPE_CVS)
            {
                result = NETWORK_ANALYSIS_CVS_FMT;
            }
            else
            {
                result = NETWORK_ANALYSIS_JSON_FMT;
            }
            break;
        case OPERATION_BINARY_SIZE:
            if (report_info->type == REPORTER_TYPE_CVS)
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
        (void)printf("Failure allocating report info\r\n");
    }
    else
    {
        //JSON_Object* json_object;
        result->type = type;
        if (result->type == REPORTER_TYPE_JSON)
        {
            STRING_HANDLE json_node = STRING_construct_sprintf(SDK_ANALYSIS_EMPTY_NODE, OS_NAME, UPLOAD_INCLUDED, LOGGING_INCLUDED);
            if (json_node == NULL)
            {
                (void)printf("Failure creating Analysis node\r\n");
                free(result);
                result = NULL;
            }
            else
            {
                result->root_value = json_parse_string(STRING_c_str(json_node));
                if (result->root_value == NULL)
                {
                    (void)printf("Failure parsing value node\r\n");
                    free(result);
                    result = NULL;
                }
                else if ((result->analysis_node = json_value_get_object(result->root_value)) == NULL)
                {
                    (void)printf("Failure getting value node\r\n");
                    free(result);
                    result = NULL;
                }
                /*else
                {
                    char* test = json_serialize_to_string_pretty(result->root_value);
                    (void)printf("Debug:\r\n%s\r\n", test);
                }*/
            }
            STRING_delete(json_node);
        }
        else
        {
        }
    }
    return result;
}

void report_deinitialize(REPORT_HANDLE handle)
{
    // Close the file
    if (handle != NULL)
    {
        json_value_free(handle->root_value);
        free(handle);
    }
}

void report_binary_sizes(REPORT_HANDLE handle, const BINARY_INFO* bin_info)
{
    if (handle != NULL)
    {
        char date_time[DATE_TIME_LEN];
        char byte_formatted[FORMAT_MAX_LEN];
        get_report_date(date_time, DATE_TIME_LEN);
        format_bytes(bin_info->binary_size, byte_formatted);

        const char* string_format = get_format_value(handle, bin_info->operation_type);
        STRING_HANDLE binary_data = STRING_construct_sprintf(string_format, date_time, get_feature_type(bin_info->feature_type),
            get_layer_type(bin_info->feature_type), bin_info->iothub_version, get_protocol_name(bin_info->iothub_protocol), byte_formatted);
        if (binary_data == NULL)
        {
            (void)printf("ERROR: Failed to allocate binary json\r\n");
        }
        else
        {
            add_node_to_json(STRING_c_str(binary_data), handle);
            STRING_delete(binary_data);
        }
    }
}

void report_memory_usage(REPORT_HANDLE handle, const MEM_ANALYSIS_INFO* iot_mem_info)
{
    if (handle != NULL)
    {
        char date_time[DATE_TIME_LEN];
        char max_use[FORMAT_MAX_LEN];
        char current_mem[FORMAT_MAX_LEN];
        char alloc_number[FORMAT_MAX_LEN];
        get_report_date(date_time, DATE_TIME_LEN);
        
        format_bytes(gballoc_getMaximumMemoryUsed(), max_use);
        format_bytes(gballoc_getCurrentMemoryUsed(), current_mem);
        format_bytes(gballoc_getAllocationCount(), alloc_number);

        const char* string_format = get_format_value(handle, iot_mem_info->operation_type);
        STRING_HANDLE analysis_data = STRING_construct_sprintf(string_format, date_time, get_feature_type(iot_mem_info->feature_type), get_layer_type(iot_mem_info->feature_type),
            iot_mem_info->iothub_version, get_protocol_name(iot_mem_info->iothub_protocol), iot_mem_info->msg_sent, max_use, current_mem, alloc_number);
        if (analysis_data == NULL)
        {
            (void)printf("ERROR: Failed to allocate memory json\r\n");
        }
        else
        {
            add_node_to_json(STRING_c_str(analysis_data), handle);
            STRING_delete(analysis_data);
        }
    }
}

void report_network_usage(REPORT_HANDLE handle, const MEM_ANALYSIS_INFO* iot_mem_info)
{
    if (handle != NULL)
    {
        char date_time[DATE_TIME_LEN];
        char bytes_sent[FORMAT_MAX_LEN];
        char num_sends[FORMAT_MAX_LEN];
        char bytes_recv[FORMAT_MAX_LEN];
        char num_recv[FORMAT_MAX_LEN];
        get_report_date(date_time, DATE_TIME_LEN);
        format_value(gbnetwork_getBytesSent(), bytes_sent);
        format_value(gbnetwork_getNumSends(), num_sends);
        format_value(gbnetwork_getBytesRecv(), bytes_recv);
        format_value(gbnetwork_getNumRecv(), num_recv);

        STRING_HANDLE network_data = STRING_construct_sprintf(NETWORK_ANALYSIS_JSON_FMT, date_time, get_feature_type(iot_mem_info->feature_type), iot_mem_info->iothub_version,
            get_protocol_name(iot_mem_info->iothub_protocol), iot_mem_info->msg_sent, bytes_sent, num_sends, bytes_recv, num_recv);
        if (network_data == NULL)
        {
            (void)printf("ERROR: Failed to allocate networking json\r\n");
        }
        else
        {
            add_node_to_json(STRING_c_str(network_data), handle);
            STRING_delete(network_data);
        }
    }
}

bool report_write(REPORT_HANDLE handle)
{
    bool result;
    if (handle == NULL)
    {
        result = false;
    }
    else
    {
        char* test = json_serialize_to_string_pretty(handle->root_value);
        (void)printf("\r\n%s\r\n", test);
        json_free_serialized_string(test);
        result = true;
    }
    return result;
}
