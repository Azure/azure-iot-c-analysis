// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "azure_c_shared_utility/strings.h"
#include "mem_reporter.h"
#include "iothub_client_version.h"

#ifdef USE_PROVISIONING_CLIENT
    #include "azure_prov_client/prov_device_client.h"
#endif

#ifdef WIN32
    static const char* EXECUTABLE_EXT = ".exe";
    static const char* BINARY_LL_PATH_FMT = "%s\\binary_info\\lower_layer\\%s%s_%s\\release\\%s%s_%s%s";
    static const char* BINARY_UL_PATH_FMT = "%s\\binary_info\\upper_layer\\%s%s_%s\\release\\%s%s_%s%s";
#else
    static const char* EXECUTABLE_EXT = "";
    static const char* BINARY_LL_PATH_FMT = "%s/binary_info/lower_layer/%s%s_%s/%s%s_%s%s";
    static const char* BINARY_UL_PATH_FMT = "%s/binary_info/upper_layer/%s%s_%s/%s%s_%s%s";
#endif

    static const char* const MQTT_BINARY_NAME = "mqtt_transport";
    static const char* const MQTT_WS_BINARY_NAME = "mqtt_ws_transport";
    static const char* const AMQP_BINARY_NAME = "amqp_transport";
    static const char* const AMQP_WS_BINARY_NAME = "amqp_ws_transport";
    static const char* const HTTP_BINARY_NAME = "http_transport";

static const char* const PROV_BINARY_NAME = "prov_";

static const char* UPPER_LAYER_SUFFIX = "ul";
static const char* LOWER_LAYER_SUFFIX = "ll";

typedef enum ARGUEMENT_TYPE_TAG
{
    ARGUEMENT_TYPE_UNKNOWN,
    ARGUEMENT_TYPE_CMAKE_DIR,
    ARGUEMENT_TYPE_OUTPUT_FILE
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

static bool is_lower_layer(FEATURE_TYPE type)
{
    bool result;
    switch (type)
    {
        case FEATURE_TELEMETRY_LL:
        case FEATURE_C2D_LL:
        case FEATURE_METHODS_LL:
        case FEATURE_TWIN_LL:
        case FEATURE_PROVISIONING_LL:
            result = true;
            break;
        default:
            result = false;
            break;
    }
    return result;
}

static int calculate_filesize(BINARY_INFO* bin_info, REPORT_HANDLE report_handle, PROTOCOL_TYPE type, const char* binary_path_fmt)
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
        const char* prov_exe = "";
        if (is_lower_layer(bin_info->feature_type) )
        {
            suffix = LOWER_LAYER_SUFFIX;
        }
        else
        {
            suffix = UPPER_LAYER_SUFFIX;
        }

        if (bin_info->feature_type == FEATURE_PROVISIONING_LL || bin_info->feature_type == FEATURE_PROVISIONING_UL)
        {
            prov_exe = PROV_BINARY_NAME;
        }

        bin_info->iothub_protocol = type;
        STRING_HANDLE filename_handle = STRING_construct_sprintf(binary_path_fmt, bin_info->cmake_dir, prov_exe, binary_name, suffix, prov_exe, binary_name, suffix, EXECUTABLE_EXT);
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

                report_binary_sizes(report_handle, bin_info);
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
            else if (argv[index][0] == '-' && (argv[index][1] == 'o' || argv[index][1] == 'O'))
            {
                argument_type = ARGUEMENT_TYPE_OUTPUT_FILE;
            }
        }
        else
        {
            switch (argument_type)
            {
            case ARGUEMENT_TYPE_CMAKE_DIR:
                bin_info->cmake_dir = argv[index];
                break;
            case ARGUEMENT_TYPE_OUTPUT_FILE:
                bin_info->output_file = argv[index];
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
    REPORT_HANDLE report_handle;
    memset(&bin_info, 0, sizeof(bin_info));

    if (parse_command_line(argc, argv, &bin_info) != 0)
    {
        (void)printf("Failure parsing command line\r\n");
        result = __LINE__;
    }
    else if (bin_info.cmake_dir == NULL)
    {
        (void)printf("Failure cmake directory command line option not supplied\r\n");
        result = __LINE__;
    }
    else if ((report_handle = report_initialize(REPORTER_TYPE_JSON)) == NULL)
    {
        (void)printf("Failure creating report handle\r\n");
        result = __LINE__;
    }
    else
    {
        bin_info.operation_type = OPERATION_BINARY_SIZE;
        bin_info.iothub_version = IoTHubClient_GetVersionString();
        bin_info.feature_type = FEATURE_TELEMETRY_LL;
#ifdef USE_MQTT
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_MQTT, BINARY_LL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_MQTT_WS, BINARY_LL_PATH_FMT);
#endif
#ifdef USE_HTTP
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_HTTP, BINARY_LL_PATH_FMT);
#endif
#ifdef USE_AMQP
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_AMQP, BINARY_LL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_AMQP_WS, BINARY_LL_PATH_FMT);
#endif

        bin_info.feature_type = FEATURE_TELEMETRY_UL;
#ifdef USE_MQTT
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_MQTT, BINARY_UL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_MQTT_WS, BINARY_UL_PATH_FMT);
#endif
#ifdef USE_HTTP
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_HTTP, BINARY_UL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_AMQP, BINARY_UL_PATH_FMT);
#endif
#ifdef USE_AMQP
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_AMQP_WS, BINARY_UL_PATH_FMT);
#endif

#ifdef USE_PROVISIONING_CLIENT
        bin_info.iothub_version = Prov_Device_GetVersionString();
        bin_info.feature_type = FEATURE_PROVISIONING_LL;
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_MQTT, BINARY_LL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_MQTT_WS, BINARY_LL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_HTTP, BINARY_LL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_AMQP, BINARY_LL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_AMQP_WS, BINARY_LL_PATH_FMT);

        bin_info.feature_type = FEATURE_PROVISIONING_UL;
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_MQTT, BINARY_UL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_MQTT_WS, BINARY_UL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_HTTP, BINARY_UL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_AMQP, BINARY_UL_PATH_FMT);
        (void)calculate_filesize(&bin_info, report_handle, PROTOCOL_AMQP_WS, BINARY_UL_PATH_FMT);
#endif

        result = 0;
    }

#ifdef _DEBUG
    (void)printf("Press any key to continue:");
    getchar();
#endif // _DEBUG

    return result;
}
