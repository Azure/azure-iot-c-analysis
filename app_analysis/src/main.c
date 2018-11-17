// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include "process_handler.h"

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
        // Create the process
        PROCESS_HANDLER_HANDLE proc_handle = process_handler_create(analysis_info.process_filename);
        if (proc_handle == NULL)
        {
            (void)printf("Failure creating process handler\r\n");
            result = __LINE__;
        }
        else
        {
            process_handler_destroy(proc_handle);
        }
    }
    return result;
}