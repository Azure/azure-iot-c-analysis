// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "dev_health_reporter.h"

#include "azure_c_shared_utility/threadapi.h"
//#include "azure_c_shared_utility/gbnetwork.h"

#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/xlogging.h"

#include "parson.h"
#include "hash_table.h"

/*#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"*/

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
static const char* const DEVICE_INFO_NODE = "deviceInfo";
static const char* const CPU_COUNT_NODE = "cpuCount";
static const char* const AVAILABLE_MEMORY_NODE = "availMemory";

static const char* const SDK_ANALYSIS_EMPTY_NODE = "{ \"version\": \"1.0.0\", \"osType\": \"%s\" }";
static const char* const NODE_OPERATING_SYSTEM = "osType";

static const char* const BINARY_SIZE_JSON_FMT = "{ \"analysisType\": \"diskSize\", \"size\" : \"%s\" }";
static const char* const HEAP_ANALYSIS_JSON_FMT = "{ \"analysisType\": \"memoryUsage\", \"description\" : \"%s\", \"threads\" : \"%s\", \"memory\" : \"%s\", \"handles\" : \"%s\", \"cpuLoad\" : \"%.02f %%\" } }";
static const char* const NETWORK_ANALYSIS_JSON_FMT = "{ \"analysisType\": \"Network\", \"description\" : \"%s\", \"msgSize\" : %d, \"transmitBytes\" : \"%s\", \"transmitCount\" : \"%s\", \"recvBytes\" : \"%s\", \"recvCount\" : \"%s\" } }";

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

typedef enum OPERATION_TYPE_TAG
{
    OPERATION_NETWORK,
    OPERATION_MEMORY,
    OPERATION_BINARY_SIZE,
} OPERATION_TYPE;

typedef struct JSON_REPORT_INFO_TAG
{
    JSON_Value* root_value;
    JSON_Object* health_object;
} JSON_REPORT_INFO;

typedef struct CSV_REPORT_INFO_TAG
{
    STRING_HANDLE csv_list;
} CSV_REPORT_INFO;

typedef struct REPORT_INFO_TAG
{
    SDK_TYPE sdk_type;
    REPORTER_TYPE rpt_type;
    HASH_TABLE_HANDLE health_item_list;
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

static void format_bytes(uint32_t bytes, char formatting[FORMAT_MAX_LEN], bool enable_format)
{
    char temp[FORMAT_MAX_LEN];
    bool is_normalized;
    uint32_t normalized_value;
    if (bytes > 1000 && enable_format)
    {
        normalized_value = bytes / 1000;
        is_normalized = true;
    }
    else
    {
        normalized_value = bytes;
        is_normalized = false;
    }
    sprintf(temp, "%u", normalized_value);
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
    if (is_normalized)
    {
        size_t len = strlen(formatting);
        formatting[len] = ' ';
        formatting[len+1] = 'k';
    }
}

static void add_node_to_csv(const char* field_data, const REPORT_INFO* report_info)
{
    if (STRING_sprintf(report_info->rpt_value.csv_info.csv_list, "%s", field_data) != 0)
    {
        LogError("ERROR: Failed setting field value");
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

        if ((json_analysis = json_object_get_value(report_info->rpt_value.json_info.health_object, NODE_SDK_ANALYSIS)) == NULL)
        {
            LogError("ERROR: Failed getting node object value");
        }
        else if ((json_object = json_value_get_object(json_analysis)) == NULL)
        {
            LogError("ERROR: Failed getting object value");
        }
        else if ((base_array = json_object_get_array(json_object, NODE_BASE_ARRAY)) == NULL)
        {
            LogError("ERROR: Failed getting object value");
        }
        else
        {
            if (json_array_append_value(base_array, json_value) != JSONSuccess)
            {
                LogError("ERROR: Failed to allocate binary json");

            }
        }
    }
}

/*static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
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
        LogError("Data Succesfully uploaded to Azure IoTHub");
    }
    else
    {
        result = false;
        LogError("Failed to uploaded data to Azure IoTHub");
    }
    return result;
}*/

static int write_to_storage(const char* report_data, const char* output_file, REPORTER_TYPE rpt_type)
{
    int result;
    if (output_file != NULL)
    {
        const char* filemode = "a";
        if (rpt_type == REPORTER_TYPE_JSON)
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

/*static const char* get_protocol_name(PROTOCOL_TYPE protocol)
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
}*/

static const char* get_sdk_type(SDK_TYPE sdk_type)
{
    const char* result;
    switch (sdk_type)
    {
        case SDK_TYPE_C:
            result = "iot-sdk-c";
            break;
        case SDK_TYPE_CSHARP:
            result = "iot-sdk-csharp";
            break;
        case SDK_TYPE_JAVA:
            result = "iot-sdk-java";
            break;
        case SDK_TYPE_NODE:
            result = "iot-sdk-node";
            break;
        default:
            result = UNKNOWN_TYPE;
            break;
    }
    return result;
}

static const char* get_operation_type(OPERATION_TYPE rpt_type)
{
    const char* result;
    switch (rpt_type)
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

static const char* get_format_value(const REPORT_INFO* report_info, OPERATION_TYPE rpt_type)
{
    const char* result;
    switch (rpt_type)
    {
        case OPERATION_MEMORY:
            if (report_info->rpt_type == REPORTER_TYPE_CSV)
            {
                result = HEAP_ANALYSIS_CSV_FMT;
            }
            else
            {
                result = HEAP_ANALYSIS_JSON_FMT;
            }
            break;
        case OPERATION_NETWORK:
            if (report_info->rpt_type == REPORTER_TYPE_CSV)
            {
                result = NETWORK_ANALYSIS_CSV_FMT;
            }
            else
            {
                result = NETWORK_ANALYSIS_JSON_FMT;
            }
            break;
        case OPERATION_BINARY_SIZE:
            if (report_info->rpt_type == REPORTER_TYPE_CSV)
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

static int construct_device_health_node(JSON_Object* object_node, const DEVICE_HEALTH_INFO* device_info)
{
    int result;
    JSON_Value* value_node = json_value_init_object();
    if (value_node == NULL)
    {
        LogError("Failure initializing device info object");
        result = __FAILURE__;
    }
    else
    {
        JSON_Object* dev_info_obj = json_value_get_object(value_node);

        if (json_object_set_number(dev_info_obj, CPU_COUNT_NODE, device_info->cpu_count) != JSONSuccess)
        {
            LogError("Failure setting cpu count node");
            result = __FAILURE__;
        }
        else if (json_object_set_number(dev_info_obj, AVAILABLE_MEMORY_NODE, device_info->memory_amt) != JSONSuccess)
        {
            LogError("Failure setting available memory node");
            result = __FAILURE__;
        }
        else if (json_object_set_value(object_node, DEVICE_INFO_NODE, value_node) != JSONSuccess)
        {
            LogError("Failure setting json object");
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

HEALTH_REPORTER_HANDLE health_reporter_init(REPORTER_TYPE rpt_type, SDK_TYPE sdk_type, const DEVICE_HEALTH_INFO* device_info)
{
    REPORT_INFO* result;
    if ((result = (REPORT_INFO*)malloc(sizeof(REPORT_INFO))) == NULL)
    {
        // Report failure
        LogError("Failure allocating report info");
    }
    else if ((result->health_item_list = hash_table_create(32, NULL, NULL)) == NULL)
    {
        LogError("Failure initializing health item list");

    }
    else
    {
        //char date_time[DATE_TIME_LEN];
        //get_report_date(date_time, DATE_TIME_LEN);
        // , get_protocol_name(protocol) 
        // , get_sdk_type(result->sdk_type) 

        //JSON_Object* json_object;
        result->rpt_type = rpt_type;
        result->sdk_type = sdk_type;
        if (result->rpt_type == REPORTER_TYPE_JSON)
        {
            STRING_HANDLE json_node = STRING_construct_sprintf(SDK_ANALYSIS_EMPTY_NODE, OS_NAME, "2");
            if (json_node == NULL)
            {
                LogError("Failure creating Analysis node");
                free(result);
                result = NULL;
            }
            else
            {
                result->rpt_value.json_info.root_value = json_parse_string(STRING_c_str(json_node));
                if (result->rpt_value.json_info.root_value == NULL)
                {
                    LogError("Failure parsing value node");
                    free(result);
                    result = NULL;
                }
                else if ((result->rpt_value.json_info.health_object = json_value_get_object(result->rpt_value.json_info.root_value)) == NULL)
                {
                    LogError("Failure getting value node");
                    json_value_free(result->rpt_value.json_info.root_value);
                    free(result);
                    result = NULL;
                }
                else if (construct_device_health_node(result->rpt_value.json_info.health_object, device_info) != 0)
                {
                    LogError("Failure getting value node");
                    json_value_free(result->rpt_value.json_info.root_value);
                    free(result);
                    result = NULL;
                }
                /*else
                {
                    char* test = json_serialize_to_string_pretty(result->json_info.root_value);
                    LogError("Debug:\r\n%s\r\n", test);
                }*/
            }
            STRING_delete(json_node);
        }
        else if (result->rpt_type == REPORTER_TYPE_CSV)
        {
            if ((result->rpt_value.csv_info.csv_list = STRING_new()) == NULL)
            {
                LogError("Failure creating string value for csv");
                free(result);
                result = NULL;
            }
        }
        else
        {
            LogError("Failure report mode not supported");
            free(result);
            result = NULL;
        }
    }
    return result;
}

void health_reporter_deinit(HEALTH_REPORTER_HANDLE handle)
{
    // Close the file
    if (handle != NULL)
    {
        if (handle->rpt_type == REPORTER_TYPE_JSON)
        {
            json_value_free(handle->rpt_value.json_info.root_value);
        }
        else if (handle->rpt_type == REPORTER_TYPE_CSV)
        {
            STRING_delete(handle->rpt_value.csv_info.csv_list);
        }
        free(handle);
    }
}

int health_reporter_register_health_item(HEALTH_REPORTER_HANDLE handle, uint32_t item_index, HEALTH_REPORTER_CONSTRUCT_JSON construct_json)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid argument: handle value NULL");
        result = __FAILURE__;
    }
    else if (hash_table_add_item(handle->health_item_list, item_index, construct_json) != 0)
    {
        LogError("Unable to add health item");
        result = __FAILURE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

int health_reporter_process_health_run(HEALTH_REPORTER_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid argument: handle value NULL");
        result = __FAILURE__;
    }
    else
    {

    }
    return result;
}

void report_binary_sizes(HEALTH_REPORTER_HANDLE handle, const char* description, const EXECUTABLE_INFO* exe_info)
{
    if (handle != NULL)
    {
        char byte_formatted[FORMAT_MAX_LEN];
        format_bytes((uint32_t)exe_info->app_size, byte_formatted, false);

        const char* string_format = get_format_value(handle, OPERATION_BINARY_SIZE);
        STRING_HANDLE binary_data = STRING_construct_sprintf(string_format, byte_formatted);
        if (binary_data == NULL)
        {
            LogError("ERROR: Failed to allocate binary json");
        }
        else
        {
            if (handle->rpt_type == REPORTER_TYPE_CSV)
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

void report_memory_usage(HEALTH_REPORTER_HANDLE handle, const char* description, const PROCESS_INFO* process_info)
{
    if (handle != NULL)
    {
        char threads[FORMAT_MAX_LEN];
        char memory[FORMAT_MAX_LEN];
        char handles[FORMAT_MAX_LEN];
        
        format_bytes(process_info->num_threads, threads, false);
        format_bytes(process_info->memory_size, memory, false);
        format_bytes(process_info->handle_cnt, handles, false);

        const char* string_format = get_format_value(handle, OPERATION_MEMORY);
        STRING_HANDLE analysis_data = STRING_construct_sprintf(string_format, description, threads, memory, handles, process_info->cpu_load);
        if (analysis_data == NULL)
        {
            LogError("ERROR: Failed to allocate memory json");
        }
        else
        {
            if (handle->rpt_type == REPORTER_TYPE_CSV)
            {
                add_node_to_csv(STRING_c_str(analysis_data), handle);
            }
            else
            {
                add_node_to_json(STRING_c_str(analysis_data), handle);
            }
            STRING_delete(analysis_data);
        }
    }
}

void report_network_usage(HEALTH_REPORTER_HANDLE handle, const char* description, const NETWORK_INFO* network_info)
{
    if (handle != NULL)
    {
        char bytes_sent[FORMAT_MAX_LEN];
        char num_sends[FORMAT_MAX_LEN];
        char bytes_recv[FORMAT_MAX_LEN];
        char num_recv[FORMAT_MAX_LEN];
        format_value(network_info->bytes_transmit, bytes_sent);
        format_value(network_info->packets_transmit, num_sends);
        format_value(network_info->bytes_recv, bytes_recv);
        format_value(network_info->packets_recv, num_recv);

        STRING_HANDLE network_data = STRING_construct_sprintf(NETWORK_ANALYSIS_JSON_FMT, description, network_info->msg_size, bytes_sent, num_sends, bytes_recv, num_recv);
        if (network_data == NULL)
        {
            LogError("ERROR: Failed to allocate networking json");
        }
        else
        {
            if (handle->rpt_type == REPORTER_TYPE_CSV)
            {
                add_node_to_csv(STRING_c_str(network_data), handle);
            }
            else
            {
                add_node_to_json(STRING_c_str(network_data), handle);
            }
            STRING_delete(network_data);
        }
    }
}

bool report_write(HEALTH_REPORTER_HANDLE handle, const char* output_file, const char* conn_string)
{
    bool result;
    if (handle == NULL)
    {
        result = false;
    }
    else
    {
        if (handle->rpt_type == REPORTER_TYPE_JSON)
        {
            char* report_data = json_serialize_to_string_pretty(handle->rpt_value.json_info.root_value);
            if (report_data == NULL)
            {
                LogError("Failed serializing json");
                result = false;
            }
            else
            {
                // Write to a file or print out string
                if (output_file != NULL)
                {
                    write_to_storage(report_data, output_file, handle->rpt_type);
                }
                else
                {
                    LogInfo("%s", report_data);
                }

                if (conn_string != NULL)
                {
                    //upload_to_azure(conn_string, report_data);
                }
                json_free_serialized_string(report_data);
            }
        }
        else
        {
            const char* report_data = STRING_c_str(handle->rpt_value.csv_info.csv_list);
            if (output_file != NULL)
            {
                write_to_storage(report_data, output_file, handle->rpt_type);
            }
            if (conn_string != NULL)
            {
                //upload_to_azure(conn_string, report_data);
            }
        }
        result = true;
    }
    return result;
}
