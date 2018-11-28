// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define _GNU_SOURCE
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
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

    char process_state;

    uint32_t virt_memory_size;
    uint32_t memory_size;
    uint32_t num_threads;
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
        if ((fgets(pid_line, PID_LINE_LENGTH, fp) == NULL) || (pid = strtok(pid_line, " ")) == NULL)
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

// http://man7.org/linux/man-pages/man5/proc.5.html
static int get_process_stat(PROCESS_HANDLER_HANDLE handle, PROCESS_INFO* proc_info)
{
    int result;
    char proc_file[128];
    sprintf(proc_file, "/proc/%d/stat", handle->proc_id);

    FILE* stat_info = fopen(proc_file, "r");
    if (stat_info == NULL)
    {
        result = __LINE__;
    }
    else
    {
        int pid;
        char filename[128];
        char state;
        int parent_pid, proc_grpid, session_id, tty_nr, tpgid;
        unsigned int flags;
        unsigned long min_faults, num_min_ft, maj_faults, num_maj_ft, utime, sched_time, virt_mem_size;
        long cu_time, cs_time, dummy, num_treads;
        unsigned long long start_time;

        if (fscanf(stat_info, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %llu %lu", &pid, filename, &state, &parent_pid, &proc_grpid, &session_id, &tty_nr,
        &tpgid, &flags, &min_faults, &num_min_ft, &maj_faults, &num_maj_ft, &utime, &sched_time, &cu_time, &cs_time, &dummy, &dummy, &num_treads, &dummy, &start_time, &virt_mem_size) == 0)
        {
            result = __LINE__;
        }
        else
        {
            handle->process_state = state;
            proc_info->num_threads = (uint32_t)num_treads;
            proc_info->memory_size = (uint32_t)virt_mem_size;
            result = 0;
        }
        fclose(stat_info);
    }
    return result;
}

static int get_network_stat(PROCESS_HANDLER_HANDLE handle, NETWORK_INFO* network_info)
{
    int result;
    char proc_file[128];
    sprintf(proc_file, "/proc/%d/net/dev", handle->proc_id);

    FILE* stat_info = fopen(proc_file, "r");
    if (stat_info == NULL)
    {
        result = __LINE__;
    }
    else
    {
        result = 0;
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
            int status;
            execl(handle->process_filename, cmdline_args, (char*)NULL);
            waitpid(pid, &status, 0);
            exit(status);
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
        if (get_process_id(handle->process_filename) == handle->proc_id)
        {
            kill(handle->proc_id, SIGTERM);
            sleep(2);
            kill(handle->proc_id, SIGKILL);
        }
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
    else if (handle->process_state != 'R')
    {
        result = false;
    }
    else
    {
        result = (get_process_id(handle->process_filename) != 0);
    }
    return result;
}

int process_handler_get_process_info(PROCESS_HANDLER_HANDLE handle, PROCESS_INFO* proc_info)
{
    int result;
    if (handle == NULL || proc_info == NULL)
    {
        LogError("Invalid argument handle: %p proc_info: %p", handle, proc_info);
        result = __LINE__;
    }
    else
    {
        if (get_process_stat(handle, proc_info) == 0)
        {
            result = 0;
        }
        else
        {
            result = handle->memory_size;
        }
    }
    return result;
}

int process_handler_get_network_info(PROCESS_HANDLER_HANDLE handle, NETWORK_INFO* network_info)
{
    int result;
    if (handle == NULL || network_info == NULL)
    {
        LogError("Invalid argument handle: %p network_info: %p", handle, network_info);
        result = __LINE__;
    }
    else
    {
        result = 0;
    }
    return result;
} 


/*uint32_t process_handler_get_memory_used(PROCESS_HANDLER_HANDLE handle)
{
    uint32_t result;
    if (handle == NULL)
    {
        result = 0;
    }
    else
    {
        if (get_process_stat(handle) == 0)
        {
            result = 0;
        }
        else
        {
            result = handle->memory_size;
        }
    }
    return result;
}

uint32_t process_handler_get_threads(PROCESS_HANDLER_HANDLE handle)
{
    uint32_t result;
    if (handle == NULL)
    {
        result = 0;
    }
    else
    {
        if (get_process_stat(handle) == 0)
        {
            result = 0;
        }
        else
        {
            result = handle->num_threads;
        }
    }
    return result;
}*/
