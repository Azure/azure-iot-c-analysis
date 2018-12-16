// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/tickcounter.h"

#define LOOP_COUNT          4200
#define ALLOC_SIZE          1024*1000 // one K
#define RUN_TIME_SEC        50

static const char* MEMORY_VALUE_NAME = "Pss";
static const size_t MEMORY_VALUE_LEN = 3;
static uint32_t calculate_memory_usage(pid_t proc_id)
{
    uint32_t result = 0;
    char proc_file[128];
    sprintf(proc_file, "/proc/%d/smaps", proc_id);
    FILE* smaps_info = fopen(proc_file, "r");
    if (smaps_info == NULL)
    {
        LogError("failure opening system smaps file");
        result = 0;
    }
    else
    {
        size_t len = 0;
        char* smaps_line = NULL;
        ssize_t read_amt;
        while ((read_amt = getline(&smaps_line, &len, smaps_info)) != -1)
        {
            if (read_amt > 3)
            {
                if (strncmp(smaps_line, MEMORY_VALUE_NAME, MEMORY_VALUE_LEN) == 0)
                {
                    const char* iterator = smaps_line + MEMORY_VALUE_LEN;
                    do 
                    {
                        if (*iterator >= 48 && *iterator <= 57)
                        {
                            uint32_t curr_line = atol(iterator);
                            result += curr_line;
                            break;
                        }
                        iterator++;
                    } while (iterator != NULL && *iterator != '\n');
                }
            }
        }
        free(smaps_line);
        fclose(smaps_info);
    }
    return result;
}

int main(int argc, char* argv[])
{
    pid_t curr_pid = getpid();
    TICK_COUNTER_HANDLE tickcounter_handle;

    if ((tickcounter_handle = tickcounter_create()) == NULL)
    {
        (void)printf("Failure creating tick counter\r\n");
    }
    else
    {
        uint32_t mem_value = calculate_memory_usage(curr_pid);

        //char* test_value = (char*)malloc(ALLOC_SIZE);
        //memset(test_value, '@', sizeof(ALLOC_SIZE));

        tickcounter_ms_t last_poll_time = 0;
        tickcounter_ms_t current_time = 0;
        size_t index = 1;

        (void)printf("test application will running for %d seconds\r\n", RUN_TIME_SEC);
        long temp_value;

        tickcounter_get_current_ms(tickcounter_handle, &last_poll_time);

        while (((current_time - last_poll_time)/1000) < RUN_TIME_SEC)
        {
            tickcounter_get_current_ms(tickcounter_handle, &current_time);

            mem_value = calculate_memory_usage(curr_pid);
            (void)printf("Mem usage: %u\r\n", mem_value);

            //for (size_t inner = 0; inner < 50; inner++)
            //{
            //    temp_value = 25654 % 32;
            //}
            //if ((current_time - last_poll_time)/1000 < RUN_TIME_SEC/2)
            //{
            //    (void)printf("Half way complete\r\n");
            //}

            ThreadAPI_Sleep(1500);
            index++;
        }
        //free(test_value);
        (void)printf("Finished test application\r\n");
    }
    return 0;
}
