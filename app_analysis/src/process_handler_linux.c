// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "process_handler.h"

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"

typedef struct PROCESS_HANDLER_INFO_TAG
{
    char* process_filename;
    pid_t proc_id;
    PROCESS_END_CB process_end_cb;
    void* user_cb;
} PROCESS_HANDLER_INFO;

#define PID_LINE_LENGTH     1024

static int get_process_id(const char* process_name)
{
    int result;
    char pid_line[PID_LINE_LENGTH];
    
    FILE* fp = popen("pidof test_app", "r");
    if (fp == NULL)
    {
        result = 0;
    }
    else
    {
        char* pid;
        fgets(pid_line, PID_LINE_LENGTH, fp);
        if ((pid = strtok(pid_line, " ")) == NULL)
        {

            result = 0;
        }
        else
        {
            result = atoi(pid);
        }
        pclose(fp);
    }
    return result;
}

PROCESS_HANDLER_HANDLE process_handler_create(const char* process_path, PROCESS_END_CB process_end_cb, void* user_cb)
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
        }
    }
    return result;
}

PROCESS_HANDLER_HANDLE process_handler_create_by_pid(int process_id, PROCESS_END_CB process_end_cb, void* user_cb)
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
        pid_t pid;
        if ((pid = fork()) == -1)
        {
            result = __LINE__;
        }
        else if (pid == 0)
        {
            exec(handle->process_filename, cmdline_args);
            exit(0);
        }
        else
        {
            // Give the executable a chance to start
            ThreadAPI_Sleep(2000);
            if ((handle->proc_id = get_process_id(handle->process_filename)) == 0)
            {
                result = __LINE__;
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}

int process_handler_end(PROCESS_HANDLER_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid argument Process Handler is NULL");
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
}

bool process_handler_is_active(PROCESS_HANDLER_HANDLE handle)
{
    bool result;
    if (handle == NULL)
    {
        result = false;
    }
    else
    {
        result = (get_process_id(handle->process_filename) != 0);
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

