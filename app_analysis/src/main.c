// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef WIN32
#include <vld.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include "process_handler.h"
#include "binary_handler.h"
#include "mem_reporter.h"

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"

#define TOLOWER(c) (((c>='A') && (c<='Z'))?c-'A'+'a':c)

#define PROCESS_MEMORY_READ_TIME_SEC    1

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

static int report_data(SDK_TYPE sdk_type)
{
    int result;

    REPORT_HANDLE rpt_handle = report_initialize(REPORTER_TYPE_JSON, sdk_type);
    if (rpt_handle == NULL)
    {
        result = __LINE__;
    }
    else
    {
        //report_memory_usage(rpt_handle, const MEM_ANALYSIS_INFO* iot_mem_info);
        report_deinitialize(rpt_handle);
        result = 0;
    }

    return result;
}

int main(int argc, char* argv[])
{
    int result;
    ANALYSIS_INFO analysis_info;
    TICK_COUNTER_HANDLE tickcounter_handle;

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
    else if ((tickcounter_handle = tickcounter_create()) == NULL)
    {
        (void)printf("Failure creating tick counter\r\n");
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
            tickcounter_ms_t last_poll_time = 0;
            tickcounter_ms_t current_time = 0;
            tickcounter_get_current_ms(tickcounter_handle, &last_poll_time);

            if (process_handler_start(proc_handle, analysis_info.process_arguments) != 0)
            {
                (void)printf("Failure starting process handler\r\n");
                result = __LINE__;
            }
            else
            {
                PROCESS_INFO proc_info_min = { 0 };
                PROCESS_INFO proc_info_max = { 0 };
                PROCESS_INFO proc_info_avg = { 0 };
                size_t iteration = 0;
                do
                {
                    tickcounter_get_current_ms(tickcounter_handle, &current_time);
                    if ((current_time - last_poll_time) / 1000 > PROCESS_MEMORY_READ_TIME_SEC)
                    {
                        PROCESS_INFO proc_info;
                        last_poll_time = current_time;
                        iteration++;
                        if (process_handler_get_process_info(proc_handle, &proc_info) == 0)
                        {
                            (void)printf("mem: %d threads: %d, handle: %d\r\n", proc_info.memory_size, proc_info.num_threads, proc_info.handle_cnt);

                            // Calculate the min values
                            proc_info.handle_cnt = proc_info.handle_cnt < proc_info_min.handle_cnt ? proc_info.handle_cnt : proc_info_min.handle_cnt;
                            proc_info.num_threads = proc_info.num_threads < proc_info_min.num_threads ? proc_info.num_threads : proc_info_min.num_threads;
                            proc_info.memory_size = proc_info.memory_size < proc_info_min.memory_size ? proc_info.memory_size : proc_info_min.memory_size;

                            // Calculate max values
                            proc_info.handle_cnt = proc_info.handle_cnt > proc_info_min.handle_cnt ? proc_info.handle_cnt : proc_info_min.handle_cnt;
                            proc_info.num_threads = proc_info.num_threads > proc_info_min.num_threads ? proc_info.num_threads : proc_info_min.num_threads;
                            proc_info.memory_size = proc_info.memory_size > proc_info_min.memory_size ? proc_info.memory_size : proc_info_min.memory_size;
                        }
                    }
                    ThreadAPI_Sleep(10);
                } while (process_handler_is_active(proc_handle));
                process_handler_end(proc_handle);
                result = 0;
            }
            process_handler_destroy(proc_handle);
        }
        tickcounter_destroy(tickcounter_handle);
    }

    (void)printf("Press any key to continue:");
    (void)getchar();
    return result;
}