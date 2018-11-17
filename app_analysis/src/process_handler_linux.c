// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>

#include "process_handler.h"

#include "azure_c_shared_utility/crt_abstractions.h"

typedef struct PROCESS_HANDLER_INFO_TAG
{
    char* process_filename;
    __pid_t proc_id;
} PROCESS_HANDLER_INFO;

PROCESS_HANDLER_HANDLE process_handler_create(const char* process_path)
{
    PROCESS_HANDLER_INFO* result;
    if (process_path == NULL)
    {
        result = NULL;
    }
    else if ((result = (PROCESS_HANDLER_INFO*)malloc(sizeof(PROCESS_HANDLER_INFO))) != NULL)
    {
        memset(result, 0, sizeof(PROCESS_HANDLER_INFO));
        if (mallocAndStrcpy_s(&result->process_filename, process_path) != 0)
        {
            free(result);
            result = NULL;
        }
        else
        {
            char pidline[1024];
            char *pid;
            FILE* fp = popen("pidof test_app", "r");
            int i = 0;
            int pidno[64];
            
            fgets(pidline, 1024, fp);
            printf("%s", pidline);
            pid = strtok (pidline, " ");
            while (pid != NULL)
            {
                pidno[i] = atoi(pid);
                printf("%d\n",pidno[i]);
                pid = strtok (NULL , " ");
                i++;
            }
            pclose(fp);
        }
    }
    return result;
}

PROCESS_HANDLER_HANDLE process_handler_create_by_pid(int process_id)
{
    PROCESS_HANDLER_INFO* result;
    if ((result = (PROCESS_HANDLER_INFO*)malloc(sizeof(PROCESS_HANDLER_INFO))) != NULL)
    {
        memset(result, 0, sizeof(PROCESS_HANDLER_INFO));
        result->proc_id = process_id;
    }
    return result;
}

void process_handler_destroy(PROCESS_HANDLER_HANDLE handle)
{
    if (handle != NULL)
    {
        free(handle);
    }
}

int process_handler_start(PROCESS_HANDLER_HANDLE handle, const char* cmdline_args)
{
    int result;
    if (handle == NULL)
    {
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

uint32_t process_handler_get_bin_size(PROCESS_HANDLER_HANDLE handle)
{
    uint32_t result;
    if (handle == NULL)
    {
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

uint32_t process_handler_get_memory_used(PROCESS_HANDLER_HANDLE handle)
{
    uint32_t result;
    if (handle == NULL)
    {
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

uint32_t process_handler_get_threads(PROCESS_HANDLER_HANDLE handle)
{
    uint32_t result;
    if (handle == NULL)
    {
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

