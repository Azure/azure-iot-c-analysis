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
#include <dirent.h>

#include "process_handler.h"

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/strings.h"

static const char* MEMORY_VALUE_NAME = "Pss";
static const size_t MEMORY_VALUE_LEN = 3;

typedef struct PROCESS_HANDLER_INFO_TAG
{
    SDK_TYPE sdk_type;
    char* process_filename;
    const char* filename;
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
    
    char pidof_value[128];
    sprintf(pidof_value, "pidof %s", process_name);
    FILE* fp = popen(pidof_value, "r");
    if (fp == NULL)
    {
        result = 0;
    }
    else
    {
        char pid_line[PID_LINE_LENGTH];
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

/*static uint32_t get_cpu_usage(const pid_t pid, struct pstat* result)
{
    //convert  pid to string
    char pid_s[20];
    snprintf(pid_s, sizeof(pid_s), "%d", pid);
    char stat_filepath[30] = "/proc/";
    strncat(stat_filepath, pid_s, sizeof(stat_filepath) - strlen(stat_filepath) -1);
    strncat(stat_filepath, "/stat", sizeof(stat_filepath) - strlen(stat_filepath) -1);

    FILE *fpstat = fopen(stat_filepath, "r");
    if (fpstat == NULL)
    {
        perror("FOPEN ERROR ");
        return -1;
    }

    FILE *fstat = fopen("/proc/stat", "r");
    if (fstat == NULL)
    {
        perror("FOPEN ERROR ");
        fclose(fstat);
        return -1;
    }

    //read values from /proc/pid/stat
    bzero(result, sizeof(struct pstat));
    long int rss;
    if (fscanf(fpstat, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu"
                "%lu %ld %ld %*d %*d %*d %*d %*u %lu %ld",
                &result->utime_ticks, &result->stime_ticks,
                &result->cutime_ticks, &result->cstime_ticks, &result->vsize,
                &rss) == EOF) {
        fclose(fpstat);
        return -1;
    }
    fclose(fpstat);
    result->rss = rss * getpagesize();

    //read+calc cpu total time from /proc/stat
    long unsigned int cpu_time[10];
    bzero(cpu_time, sizeof(cpu_time));
    if (fscanf(fstat, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                &cpu_time[0], &cpu_time[1], &cpu_time[2], &cpu_time[3],
                &cpu_time[4], &cpu_time[5], &cpu_time[6], &cpu_time[7],
                &cpu_time[8], &cpu_time[9]) == EOF) {
        fclose(fstat);
        return -1;
    }

    fclose(fstat);

    for(int i=0; i < 10;i++)
        result->cpu_total_time += cpu_time[i];

    return 0;
}*/

static uint32_t get_total_cpu_time(void)
{
    uint32_t result;
    FILE* stat_info = fopen("/proc/stat", "r");
    if (stat_info == NULL)
    {
        LogError("failure opening system stat file");
        result = 0;
    }
    else
    {
        long unsigned int cpu_time[10];
        if (fscanf(stat_info, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &cpu_time[0], &cpu_time[1], &cpu_time[2], &cpu_time[3],
                &cpu_time[4], &cpu_time[5], &cpu_time[6], &cpu_time[7], &cpu_time[8], &cpu_time[9]) == EOF)
        {
            LogError("failure formatting system stat file");
            result = 0;
        }
        else
        {
            for (size_t index = 0; index < 10; index++)
            {
                result += cpu_time[index];
            }
        }
        fclose(stat_info);
    }
    return result;
}

static uint32_t get_total_handles(pid_t proc_id)
{
    uint32_t result = 0;
    DIR* dirp;
    struct dirent* entry;
    char proc_file[128];

    sprintf(proc_file, "/proc/%d/fd", proc_id);
    if ((dirp = opendir(proc_file)) != NULL)
    {
        while ( (entry = readdir(dirp)) != NULL) {
            //if (entry->d_type == DT_REG)
            //{ /* If the entry is a regular file */
                result++;
            //}
        }
        closedir(dirp);
    }
    else
    {
        result = 0;
    }
    return result;
}

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

// http://man7.org/linux/man-pages/man5/proc.5.html
static int get_process_stat(PROCESS_HANDLER_INFO* handle, PROCESS_INFO* proc_info)
{
    int result;

    uint32_t total_cpu = get_total_cpu_time();
    if (total_cpu == 0)
    {
        LogError("failure getting total cpu time");
        result = __LINE__;
    }
    else if ((proc_info->memory_size = calculate_memory_usage(handle->proc_id)) == 0)
    {
        LogError("failure getting total memory size");
        result = __LINE__;
    }
    else
    {
        char proc_file[128];
        sprintf(proc_file, "/proc/%d/stat", handle->proc_id);
        FILE* stat_info = fopen(proc_file, "r");
        if (stat_info == NULL)
        {
            LogError("failure opening process stat file");
            result = __LINE__;
        }
        else
        {
            int pid;
            int parent_pid, proc_grpid, session_id, tty_nr, tpgid;
            unsigned int flags;
            unsigned long min_faults, num_min_ft, maj_faults, num_maj_ft, utime, stime, virt_mem_size;
            long cu_time, cs_time, dummy, rss;
            unsigned long long start_time;

            if (fscanf(stat_info, "%d %*s %c %*d %*d %*d %*d %*d %*u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %d %ld %llu %lu %ld", &pid, &handle->process_state,
                &min_faults, &num_min_ft, &maj_faults, &num_maj_ft, &utime, &stime, &cu_time, &cs_time, &dummy, &dummy, &proc_info->num_threads, &dummy, &start_time, &virt_mem_size, &rss) == 0)
            {
                LogError("failure formatting process stat file");
                result = __LINE__;
            }
            else
            {
                printf("virt: %lu, rss: %ld\r\n", virt_mem_size, rss);
                proc_info->cpu_load = (float)(100*(utime+stime)/total_cpu);
                proc_info->virtual_mem_size = virt_mem_size;
                result = 0;
            }
            fclose(stat_info);

            proc_info->handle_cnt = get_total_handles(handle->proc_id);
        }
    }
    return result;
}

static int get_network_stat(const PROCESS_HANDLER_INFO* handle, NETWORK_INFO* network_info)
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

static int start_process(const PROCESS_HANDLER_INFO* handle, const char* cmdline_args)
{
    int result = 0;
    int status = 0;
    char exec_file[512];

    switch (handle->sdk_type)
    {
        case SDK_TYPE_C:
        {
            if (execl(handle->process_filename, cmdline_args, (char*)NULL) == -1)
            {
                LogError("Failure executing process %s", handle->process_filename);
                result = __LINE__;
            }
            break;
        }
        case SDK_TYPE_CSHARP:
            break;
        case SDK_TYPE_JAVA:
            break;
        case SDK_TYPE_NODE:
        {
            // node <process_filename> <cmdline_args>
            if (execl("node", handle->process_filename, cmdline_args, (char*)NULL) == -1)
            {
                LogError("Failure executing script %s", handle->process_filename);
                result = __LINE__;
            }
            break;
        }
        case SDK_TYPE_PYTHON:
        {
            // python <process_filename> <cmdline_args>
            if (execl("python", handle->process_filename, cmdline_args, (char*)NULL) == -1)
            {
                LogError("Failure executing script %s", handle->process_filename);
                result = __LINE__;
            }
            break;
        }
        case SDK_TYPE_UNKNOWN:
        default:
            LogError("Failure attempt to execute unknown SDK type %d", (int)handle->sdk_type);
            result = __FAILURE__;
            break;
    }
    if (result == 0)
    {
        // Wait for the process to complete
        pid_t p_id = getpid();
        waitpid(p_id, &status, 0);
    }
    exit(status);
    return result;
}

PROCESS_HANDLER_HANDLE process_handler_create(const char* process_path, SDK_TYPE sdk_type, PROCESS_END_CB process_end_cb, void* user_cb)
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
            result->sdk_type = sdk_type;
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
            (void)start_process(handle, cmdline_args);
        }
        else
        {
            // Give the executable a chance to start
            ThreadAPI_Sleep(2000);
            if ((handle->proc_id = get_process_id(handle->filename)) == 0)
            {
                LogError("Failure getting process id of application %s", handle->process_filename);
                result = __LINE__;
            }
            else
            {
                handle->process_state = 'R';
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
        if (get_process_id(handle->filename) == handle->proc_id)
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
    else 
    {
        int status;
        pid_t cp = waitpid(handle->proc_id, &status, WNOHANG|WUNTRACED); 
        if (cp == -1)
        {
            result = false;
        }
        else
        {
            if (WIFEXITED(status))
            {
                printf("proc exited\r\n");
                result = false;
            }
            else
            {
                result = true;
            }
        }
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
        if (!process_handler_is_active(handle))
        {
            printf("Is not active\r\n");
            result = __LINE__;
        }
        else
        {
            if (get_process_stat(handle, proc_info) != 0)
            {
                result = __LINE__;
            }
            else
            {
                result = handle->memory_size;
            }
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
