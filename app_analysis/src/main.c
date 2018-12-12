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

#define DEFAULT_PROCESS_MEMORY_READ_TIME_SEC    1000

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
    SDK_TYPE target_sdk;
    PROTOCOL_TYPE protocol_type;
} ANALYSIS_INFO;

typedef struct EXEC_INFO_TAG
{
    uint32_t bin_size;
    uint32_t memory_max;
    uint16_t thread_cnt;
} EXEC_INFO;

typedef struct ANALYSIS_RUN_TAG
{
    EXEC_INFO exec_info;
    PROCESS_INFO proc_info_min;
    PROCESS_INFO proc_info_max;
    PROCESS_INFO proc_info_avg;
    NETWORK_INFO network_info;
} ANALYSIS_RUN;

static const struct
{
    char short_argument;
    const char* long_argument;
    const char* description;
    int mnemonic_value;
} OPTION_ITEMS[] =
{
    { 'p', "process", "The full path to the process", ARGUEMENT_TYPE_PROCESS_NAME },
    { 'd', "dev-conn", "The azure IoTHub connection string used to send test messages", ARGUEMENT_TYPE_CONN_STRING }
};

static void print_help(void)
{
    (void)printf("usage: app_analysis [options] [sdk_type <c/c#/java/node>]\r\n");
    for (size_t index = 0; index < sizeof(OPTION_ITEMS) / sizeof(OPTION_ITEMS[0]); index++)
    {
        (void)printf("   -%c --%s\t\t%s\r\n", OPTION_ITEMS[index].short_argument, OPTION_ITEMS[index].long_argument, OPTION_ITEMS[index].description);
    }
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
                if (argv[index][1] == '-')
                {
                    if (strcmp(argv[index], "--process") == 0)
                    {
                        argument_type = ARGUEMENT_TYPE_PROCESS_NAME;
                    }
                    else if (strcmp(argv[index], "--dev-conn") == 0)
                    {
                        argument_type = ARGUEMENT_TYPE_CONN_STRING;
                    }
                }
                else
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
                        case 'd':
                            argument_type = ARGUEMENT_TYPE_CONN_STRING;
                            break;
                        case '?':
                            break;
                    }
                }
            }
            else
            {
                if (strcmp(argv[index], "c") == 0)
                {
                    anaylsis_info->target_sdk = SDK_TYPE_C;
                }
                else if (strcmp(argv[index], "c#") == 0)
                {
                    anaylsis_info->target_sdk = SDK_TYPE_CSHARP;
                }
                else if (strcmp(argv[index], "java") == 0)
                {
                    anaylsis_info->target_sdk = SDK_TYPE_JAVA;
                }
                else if (strcmp(argv[index], "node") == 0)
                {
                    anaylsis_info->target_sdk = SDK_TYPE_NODE;
                }
                else
                {
                    if (index == argc - 1)
                    {
                        (void)printf("Unable to recognize sdk type\r\n");
                        result = __LINE__;
                    }
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

    anaylsis_info->protocol_type = PROTOCOL_MQTT;
    return result;
}

static int validate_args(ANALYSIS_INFO* anaylsis_info)
{
    int result;
    if (anaylsis_info->process_filename == NULL)
    {
        (void)printf("Process filename not found\r\n");
        result = __LINE__;
    }
    else if (anaylsis_info->target_sdk == SDK_TYPE_UNKNOWN)
    {
        (void)printf("Target SDK not found\r\n");
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

static int report_data(const ANALYSIS_INFO* analysis_info, const ANALYSIS_RUN* run_info, PROTOCOL_TYPE type)
{
    int result;

    REPORT_HANDLE rpt_handle = report_initialize(REPORTER_TYPE_JSON, analysis_info->target_sdk, type);
    if (rpt_handle == NULL)
    {
        (void)printf("Failure loading reporter object\r\n");
        result = __LINE__;
    }
    else
    {
        //
        report_memory_usage(rpt_handle, "average", &run_info->proc_info_avg);
        report_memory_usage(rpt_handle, "minimum", &run_info->proc_info_min);
        report_memory_usage(rpt_handle, "maximum", &run_info->proc_info_max);

        report_write(rpt_handle, NULL, NULL);

        report_deinitialize(rpt_handle);
        result = 0;
    }

    return result;
}

static int execute_analysis_run(const ANALYSIS_INFO* analysis_info, ANALYSIS_RUN* run_info)
{
    int result;
    TICK_COUNTER_HANDLE tickcounter_handle;

    if ((tickcounter_handle = tickcounter_create()) == NULL)
    {
        (void)printf("Failure creating tick counter\r\n");
        result = __LINE__;
    }
    else
    {
        // Create the process
        PROCESS_HANDLER_HANDLE proc_handle = process_handler_create(analysis_info->process_filename, NULL, NULL);
        if (proc_handle == NULL)
        {
            (void)printf("Failure creating process handler\r\n");
            result = __LINE__;
        }
        else
        {
            size_t read_time = DEFAULT_PROCESS_MEMORY_READ_TIME_SEC;
            tickcounter_ms_t last_poll_time = 0;
            tickcounter_ms_t current_time = 0;
            tickcounter_get_current_ms(tickcounter_handle, &last_poll_time);

            if (process_handler_start(proc_handle, analysis_info->process_arguments) != 0)
            {
                (void)printf("Failure starting process handler\r\n");
                result = __LINE__;
            }
            else
            {
                size_t iteration = 0;
                PROCESS_INFO temp_accumulator = { 0 };
                do
                {
                    tickcounter_get_current_ms(tickcounter_handle, &current_time);
                    if ((current_time - last_poll_time) > read_time)
                    {
                        PROCESS_INFO proc_info;
                        last_poll_time = current_time;
                        iteration++;
                        if (process_handler_get_process_info(proc_handle, &proc_info) == 0)
                        {
                            (void)printf("mem: %d threads: %d, handle: %d\r\n", proc_info.memory_size, proc_info.num_threads, proc_info.handle_cnt);

                            // Calculate the min values
                            if (run_info->proc_info_min.handle_cnt == 0 || proc_info.handle_cnt < run_info->proc_info_min.handle_cnt)
                            {
                                run_info->proc_info_min.handle_cnt = proc_info.handle_cnt;
                            }
                            if (run_info->proc_info_min.num_threads == 0 || proc_info.num_threads < run_info->proc_info_min.num_threads)
                            {
                                run_info->proc_info_min.num_threads = proc_info.num_threads;
                            }
                            if (run_info->proc_info_min.memory_size == 0 || proc_info.memory_size < run_info->proc_info_min.memory_size)
                            {
                                run_info->proc_info_min.memory_size = proc_info.memory_size;
                            }

                            // Calculate max values
                            if (proc_info.handle_cnt > run_info->proc_info_min.handle_cnt)
                            {
                                run_info->proc_info_min.handle_cnt = proc_info.handle_cnt;
                            }
                            if (proc_info.num_threads > run_info->proc_info_min.num_threads)
                            {
                                run_info->proc_info_min.num_threads = proc_info.num_threads;
                            }
                            if (proc_info.memory_size > run_info->proc_info_min.memory_size)
                            {
                                run_info->proc_info_min.memory_size = proc_info.memory_size;
                            }
                            temp_accumulator.handle_cnt += proc_info.handle_cnt;
                            temp_accumulator.handle_cnt += proc_info.num_threads;
                            temp_accumulator.handle_cnt += proc_info.memory_size;
                        }

                        NETWORK_INFO network_info;
                        if (process_handler_get_network_info(proc_handle, &network_info) == 0)
                        {
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
    return result;
}

int main(int argc, char* argv[])
{
    int result;
    ANALYSIS_INFO analysis_info = { 0 };

    if (parse_command_line(argc, argv, &analysis_info) != 0)
    {
        print_help();
        result = __LINE__;
    }
    else if (validate_args(&analysis_info) != 0)
    {
        (void)printf("invalid command line arguments\r\n");
        print_help();
        result = __LINE__;
    }
    else
    {
        ANALYSIS_RUN analysis_run = { 0 };
        if (analysis_info.target_sdk == SDK_TYPE_C)
        {
            analysis_run.exec_info.bin_size = binary_handler_get_size(analysis_info.process_filename, analysis_info.target_sdk);
        }

        if (execute_analysis_run(&analysis_info, &analysis_run) != 0)
        {
            result = __LINE__;
        }
        else
        {
            analysis_run.proc_info_avg.handle_cnt = 10;
            analysis_run.proc_info_avg.memory_size = 200000;
            analysis_run.proc_info_avg.num_threads = 1;

            analysis_run.proc_info_min.handle_cnt = 5;
            analysis_run.proc_info_min.memory_size = 100000;
            analysis_run.proc_info_min.num_threads = 1;

            analysis_run.proc_info_max.handle_cnt = 30;
            analysis_run.proc_info_max.memory_size = 400000;
            analysis_run.proc_info_max.num_threads = 2;

            report_data(&analysis_info, &analysis_run, analysis_info.protocol_type);
            result = 0;
        }
    }
    (void)printf("Press any key to continue:");
    (void)getchar();
    return result;
}