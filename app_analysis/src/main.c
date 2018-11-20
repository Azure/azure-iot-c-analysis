// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include "process_handler.h"
#include "binary_handler.h"

#include "azure_c_shared_utility/threadapi.h"

#define TOLOWER(c) (((c>='A') && (c<='Z'))?c-'A'+'a':c)

typedef enum ARGUEMENT_TYPE_TAG
{
    ARGUEMENT_TYPE_UNKNOWN,
    ARGUEMENT_TYPE_PROCESS_NAME,
    ARGUEMENT_TYPE_OUTPUT_FILE,
    ARGUEMENT_TYPE_PROCESS_ARG,
    ARGUEMENT_TYPE_CONN_STRING
} ARGUEMENT_TYPE;

typedef struct ANALYSIS_INFO_TAG
{
    const char* process_filename;
    const char* output_filename;
    const char* azure_conn_string;
    const char* process_arguments;
} ANALYSIS_INFO;

typedef struct EXEC_INFO_TAG
{
    SDK_TYPE sdk_type;
    uint32_t bin_size;
    uint32_t memory_max;
    uint16_t thread_cnt;
} EXEC_INFO;

static void print_help(void)
{
    (void)printf("usage: app_analysis [-p <process path>] [-c <connection string>]\r\n");
}

// -p Process path -c "<Connection String>" -a <process arguments> -o <output file path>
static int parse_command_line(int argc, char* argv[], ANALYSIS_INFO* anaylsis_info)
{
    int result = 0;
    ARGUEMENT_TYPE argument_type = ARGUEMENT_TYPE_UNKNOWN;

    for (int index = 0; index < argc; index++)
    {
        if (argument_type == ARGUEMENT_TYPE_UNKNOWN)
        {
            if (argv[index][0] == '-')
            {
                switch (TOLOWER(argv[index][1]))
                {
                    case 'p':
                        argument_type = ARGUEMENT_TYPE_PROCESS_NAME;
                        break;
                    case 'o':
                        argument_type = ARGUEMENT_TYPE_OUTPUT_FILE;
                        break;
                    case 'a':
                        argument_type = ARGUEMENT_TYPE_PROCESS_ARG;
                        break;
                    case 'c':
                        argument_type = ARGUEMENT_TYPE_CONN_STRING;
                        break;
                }
            }
        }
        else
        {
            switch (argument_type)
            {
                case ARGUEMENT_TYPE_PROCESS_NAME:
                    anaylsis_info->process_filename = argv[index];
                    break;
                case ARGUEMENT_TYPE_OUTPUT_FILE:
                    anaylsis_info->output_filename = argv[index];
                    break;
                case ARGUEMENT_TYPE_CONN_STRING:
                    anaylsis_info->azure_conn_string = argv[index];
                    break;
                case ARGUEMENT_TYPE_PROCESS_ARG:
                    anaylsis_info->process_arguments = argv[index];
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

static int validate_args(ANALYSIS_INFO* anaylsis_info)
{
    int result;
    if (anaylsis_info->process_filename == NULL)
    {
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

int main(int argc, char* argv[])
{
    int result;
    ANALYSIS_INFO analysis_info;

    if (parse_command_line(argc, argv, &analysis_info) != 0)
    {
        (void)printf("Failure parsing command line\r\n");
        print_help();
        result = __LINE__;
    }
    else if (validate_args(&analysis_info) != 0)
    {
        (void)printf("Failure parsing command line\r\n");
        result = __LINE__;
    }
    else
    {
        EXEC_INFO exec_info;
        exec_info.sdk_type = SDK_TYPE_C;

        exec_info.bin_size = binary_handler_get_size(analysis_info.process_filename, exec_info.sdk_type);

        // Create the process
        PROCESS_HANDLER_HANDLE proc_handle = process_handler_create(analysis_info.process_filename, NULL, NULL);
        if (proc_handle == NULL)
        {
            (void)printf("Failure creating process handler\r\n");
            result = __LINE__;
        }
        else
        {
            if (process_handler_start(proc_handle, analysis_info.process_arguments) != 0)
            {
                (void)printf("Failure starting process handler\r\n");
                result = __LINE__;
            }
            else
            {
                do
                {
                    uint32_t mem = process_handler_get_memory_used(proc_handle);
                    (void)printf("mem: %d\r\n", mem);
                } while (process_handler_is_active(proc_handle));

                process_handler_end(proc_handle);
                result = 0;
            }
            process_handler_destroy(proc_handle);
        }
    }
    return result;
}