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

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"

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

static const char* const BINARY_SIZE_CSV_FMT = "%s, %s, %s, %s, %s, %s";
static const char* const HEAP_ANALYSIS_CSV_FMT = "%s, %s, %s, %s, %s, %s, %s, %d, %zu, %zu, %zu";
static const char* const NETWORK_ANALYSIS_CSV_FMT = "%s, %s, %s, %s, %s, %s, %d, %" PRIu64 ", %ld, %" PRIu64 ", %ld";

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

typedef struct JSON_REPORT_INFO_TAG
{
    JSON_Value* root_value;
    JSON_Object* analysis_node;
} JSON_REPORT_INFO;

typedef struct CSV_REPORT_INFO_TAG
{
    STRING_HANDLE csv_list;
} CSV_REPORT_INFO;

typedef struct REPORT_INFO_TAG
{
    REPORTER_TYPE type;
    union
    {
        JSON_REPORT_INFO json_info;
        CSV_REPORT_INFO csv_info;
    } rpt_value;
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

static void add_node_to_csv(const char* field_data, const REPORT_INFO* report_info)
{
    if (STRING_sprintf(report_info->rpt_value.csv_info.csv_list, "%s\r\n", field_data) != 0)
    //if (STRING_concat(report_info->rpt_value.csv_info.csv_list, field_data) != 0)
    {
        (void)printf("ERROR: Failed setting field value\r\n");
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

        if ((json_analysis = json_object_get_value(report_info->rpt_value.json_info.analysis_node, NODE_SDK_ANALYSIS)) == NULL)
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

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    int* msg_delievered = (int*)userContextCallback;
    if (msg_delievered != NULL)
    {
        if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
        {
            *msg_delievered = 1;
        }
        else
        {
            *msg_delievered = 2;
        }
    }
    // When a message is sent this callback will get envoked
}

static bool upload_to_azure(const char* connection_string, const char* data)
{
    bool result;
    int msg_delivered = 0;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE dev_ll_handle;
    (void)IoTHub_Init();

    dev_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connection_string, MQTT_Protocol);
    if (dev_ll_handle != NULL)
    {
        IOTHUB_MESSAGE_HANDLE message_handle;
        message_handle = IoTHubMessage_CreateFromString(data);
        if (message_handle != NULL)
        {
            if (IoTHubDeviceClient_LL_SendEventAsync(dev_ll_handle, message_handle, send_confirm_callback, &msg_delivered) == IOTHUB_CLIENT_OK)
            {
                do
                {
                    IoTHubDeviceClient_LL_DoWork(dev_ll_handle);
                } while (msg_delivered == 0);
            }
            IoTHubMessage_Destroy(message_handle);
        }
        IoTHubDeviceClient_LL_Destroy(dev_ll_handle);
    }
    IoTHub_Deinit();

    if (msg_delivered == 1)
    {
        result = true;
        (void)printf("Data Succesfully uploaded to Azure IoTHub\r\n");
    }
    else
    {
        result = false;
        (void)printf("Failed to uploaded data to Azure IoTHub\r\n");
    }
    return result;
}

static int write_to_storage(const char* report_data, const char* output_file, REPORTER_TYPE type)
{
    int result;
    if (output_file == NULL)
    {
        (void)printf("%s\r\n", report_data);
        result = 0;
    }
    else
    {
        const char* filemode = "a";
        if (type == REPORTER_TYPE_JSON)
        {
            filemode = "w";
        }
        FILE* file = fopen(output_file, filemode);
        if (file != NULL)
        {
            size_t len = strlen(report_data);
            fwrite(report_data, sizeof(char), len, file);
            fclose(file);
        }
        result = 0;
    }
    return result;
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
            if (report_info->type == REPORTER_TYPE_CSV)
            {
                result = HEAP_ANALYSIS_CSV_FMT;
            }
            else
            {
                result = HEAP_ANALYSIS_JSON_FMT;
            }
            break;
        case OPERATION_NETWORK:
            if (report_info->type == REPORTER_TYPE_CSV)
            {
                result = NETWORK_ANALYSIS_CSV_FMT;
            }
            else
            {
                result = NETWORK_ANALYSIS_JSON_FMT;
            }
            break;
        case OPERATION_BINARY_SIZE:
            if (report_info->type == REPORTER_TYPE_CSV)
            {
                result = BINARY_SIZE_CSV_FMT;
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
                result->rpt_value.json_info.root_value = json_parse_string(STRING_c_str(json_node));
                if (result->rpt_value.json_info.root_value == NULL)
                {
                    (void)printf("Failure parsing value node\r\n");
                    free(result);
                    result = NULL;
                }
                else if ((result->rpt_value.json_info.analysis_node = json_value_get_object(result->rpt_value.json_info.root_value)) == NULL)
                {
                    (void)printf("Failure getting value node\r\n");
                    free(result);
                    result = NULL;
                }
                /*else
                {
                    char* test = json_serialize_to_string_pretty(result->json_info.root_value);
                    (void)printf("Debug:\r\n%s\r\n", test);
                }*/
            }
            STRING_delete(json_node);
        }
        else if (result->type == REPORTER_TYPE_CSV)
        {
            if ((result->rpt_value.csv_info.csv_list = STRING_new()) == NULL)
            {
                (void)printf("Failure creating string value for csv\r\n");
                free(result);
                result = NULL;
            }
        }
        else
        {
            (void)printf("Failure report mode not supported\r\n");
            free(result);
            result = NULL;
        }
    }
    return result;
}

void report_deinitialize(REPORT_HANDLE handle)
{
    // Close the file
    if (handle != NULL)
    {
        if (handle->type == REPORTER_TYPE_JSON)
        {
            json_value_free(handle->rpt_value.json_info.root_value);
        }
        else if (handle->type == REPORTER_TYPE_CSV)
        {
            STRING_delete(handle->rpt_value.csv_info.csv_list);
        }
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
            if (bin_info->rpt_type == REPORTER_TYPE_CSV)
            {
                add_node_to_csv(STRING_c_str(binary_data), handle);
            }
            else
            {
                add_node_to_json(STRING_c_str(binary_data), handle);
            }
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

bool report_write(REPORT_HANDLE handle, const char* output_file, const char* conn_string)
{
    bool result;
    if (handle == NULL)
    {
        result = false;
    }
    else
    {
        if (handle->type == REPORTER_TYPE_JSON)
        {
            char* report_data = json_serialize_to_string_pretty(handle->rpt_value.json_info.root_value);
            if (output_file != NULL)
            {
                write_to_storage(report_data, output_file, handle->type);
            }
            if (conn_string != NULL)
            {
                upload_to_azure(conn_string, report_data);
            }
            json_free_serialized_string(report_data);
        }
        else
        {
            const char* report_data = STRING_c_str(handle->rpt_value.csv_info.csv_list);
            if (output_file != NULL)
            {
                write_to_storage(report_data, output_file, handle->type);
            }
            if (conn_string != NULL)
            {
                upload_to_azure(conn_string, report_data);
            }
        }
        result = true;
    }
    return result;
}
