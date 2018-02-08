// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>

#include "azure_c_shared_utility/strings.h"
#include "iothub_client_version.h"
#include "mem_reporter.h"

#ifdef WIN32
    static const char* EXECUTABLE_EXT = ".exe";
    static const char* BINARY_LL_PATH_FMT = "%s\\binary_info\\lower_layer\\%s_%s\\release\\%s_%s%s";
    static const char* BINARY_UL_PATH_FMT = "%s\\binary_info\\upper_layer\\%s_%s\\release\\%s_%s%s";
#else
    static const char* EXECUTABLE_EXT = "";
    static const char* BINARY_LL_PATH_FMT = "%s/binary_info/lower_layer/%s_%s/%s_%s%s";
    static const char* BINARY_UL_PATH_FMT = "%s/binary_info/upper_layer/%s_%s/%s_%s%s";
#endif

static const char* MQTT_BINARY_NAME = "mqtt_transport";
static const char* MQTT_WS_BINARY_NAME = "mqtt_ws_transport";
static const char* AMQP_BINARY_NAME = "amqp_transport";
static const char* AMQP_WS_BINARY_NAME = "amqp_ws_transport";
static const char* HTTP_BINARY_NAME = "http_transport";

static const char* UPPER_LAYER_SUFFIX = "ul";
static const char* LOWER_LAYER_SUFFIX = "ll";

typedef enum ARGUEMENT_TYPE_TAG
{
    ARGUEMENT_TYPE_UNKNOWN,
    ARGUEMENT_TYPE_CMAKE_DIR
} ARGUEMENT_TYPE;

static const char* get_binary_file(PROTOCOL_TYPE type)
{
    const char* result;
    switch (type)
    {
        case PROTOCOL_MQTT:
            result = MQTT_BINARY_NAME;
            break;
        case PROTOCOL_MQTT_WS:
            result = MQTT_WS_BINARY_NAME;
            break;
        case PROTOCOL_HTTP:
            result = HTTP_BINARY_NAME;
            break;
        case PROTOCOL_AMQP:
            result = AMQP_BINARY_NAME;
            break;
        case PROTOCOL_AMQP_WS:
            result = AMQP_WS_BINARY_NAME;
            break;
        default:
        case PROTOCOL_UNKNOWN:
            result = NULL;
            break;
    }
    return result;
}

static int calculate_filesize(BINARY_INFO* bin_info, PROTOCOL_TYPE type, const char* binary_path_fmt)
{
    int result;
    FILE* target_file;
    const char* binary_name = get_binary_file(type);
    if (binary_name == NULL)
    {
        (void)printf("Failed getting binary filename\r\n");
        result = __LINE__;
    }
    else
    {
        const char* suffix;
        if (bin_info->operation_type == OPERATION_BINARY_SIZE_LL)
        {
            suffix = LOWER_LAYER_SUFFIX;
        }
        else
        {
            suffix = UPPER_LAYER_SUFFIX;
        }

        bin_info->iothub_protocol = type;
        STRING_HANDLE filename_handle = STRING_construct_sprintf(binary_path_fmt, bin_info->cmake_dir, binary_name, suffix, binary_name, suffix, EXECUTABLE_EXT);
        if (filename_handle == NULL)
        {
            (void)printf("Failed constructing filename\r\n");
            result = __LINE__;
        }
        else
        {
            target_file = fopen(STRING_c_str(filename_handle), "rb");
            if (target_file == NULL)
            {
                (void)printf("Failed opening file: %s\r\n", STRING_c_str(filename_handle));
                result = __LINE__;
            }
            else
            {
                fseek(target_file, 0, SEEK_END);
                bin_info->binary_size = ftell(target_file);
                fclose(target_file);

                report_binary_sizes(bin_info);
                result = 0;
            }
            STRING_delete(filename_handle);
        }
    }
    return result;
}

static int parse_command_line(int argc, char* argv[], BINARY_INFO* bin_info)
{
    int result = 0;
    ARGUEMENT_TYPE argument_type = ARGUEMENT_TYPE_UNKNOWN;

    for (int index = 0; index < argc; index++)
    {
        if (argument_type == ARGUEMENT_TYPE_UNKNOWN)
        {
            if (argv[index][0] == '-' && (argv[index][1] == 'c' || argv[index][1] == 'C'))
            {
                argument_type = ARGUEMENT_TYPE_CMAKE_DIR;
            }
        }
        else
        {
            switch (argument_type)
            {
            case ARGUEMENT_TYPE_CMAKE_DIR:
                bin_info->cmake_dir = argv[index];
                break;
            case ARGUEMENT_TYPE_UNKNOWN:
            default:
                result = __LINE__;
                break;
            }
            argument_type = ARGUEMENT_TYPE_UNKNOWN;
        }
    }
    return result;
}

int main(int argc, char* argv[])
{
    int result;
    BINARY_INFO bin_info;
    memset(&bin_info, 0, sizeof(bin_info));

    if (parse_command_line(argc, argv, &bin_info) != 0)
    {
        (void)printf("Failure parsing command line\r\n");
        result = __LINE__;
    }
    else
    {
        bin_info.iothub_version = IoTHubClient_GetVersionString();
        bin_info.operation_type = OPERATION_BINARY_SIZE_LL;
        (void)calculate_filesize(&bin_info, PROTOCOL_MQTT, BINARY_LL_PATH_FMT);
        (void)calculate_filesize(&bin_info, PROTOCOL_MQTT_WS, BINARY_LL_PATH_FMT);
        (void)calculate_filesize(&bin_info, PROTOCOL_HTTP, BINARY_LL_PATH_FMT);
        (void)calculate_filesize(&bin_info, PROTOCOL_AMQP, BINARY_LL_PATH_FMT);
        (void)calculate_filesize(&bin_info, PROTOCOL_AMQP_WS, BINARY_LL_PATH_FMT);

        bin_info.operation_type = OPERATION_BINARY_SIZE_UL;
        (void)calculate_filesize(&bin_info, PROTOCOL_MQTT, BINARY_UL_PATH_FMT);
        (void)calculate_filesize(&bin_info, PROTOCOL_MQTT_WS, BINARY_UL_PATH_FMT);
        (void)calculate_filesize(&bin_info, PROTOCOL_HTTP, BINARY_UL_PATH_FMT);
        (void)calculate_filesize(&bin_info, PROTOCOL_AMQP, BINARY_UL_PATH_FMT);
        (void)calculate_filesize(&bin_info, PROTOCOL_AMQP_WS, BINARY_UL_PATH_FMT);

        result = 0;
    }

#ifdef _DEBUG
    (void)printf("Press any key to continue:");
    getchar();
#endif // _DEBUG

    return result;
}
