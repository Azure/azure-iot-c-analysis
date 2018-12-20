// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <Windows.h>

#include <pdh.h>
#include <pdhmsg.h>

#include "process_handler.h"

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"

static const char* const HANDLE_COUNT_COUNTER_FMT = "\\Process(%s)\\Handle Count";
static const char* const THREAD_COUNT_COUNTER_FMT = "\\Process(%s)\\Thread Count";
static const char* const WORKING_SET_COUNTER_FMT = "\\Process(%s)\\Working Set";
static const char* const PROCESSOR_TIME_COUNTER_FMT = "\\Process(%s)\\%% Processor Time";

typedef struct PROCESS_HANDLER_INFO_TAG
{
    char* process_filename;
    char* process_name;

    PROCESS_INFORMATION proc_info;
    PROCESS_END_CB process_end_cb;
    void* user_cb;

    HQUERY query_handle;
    HCOUNTER handle_counter;
    HCOUNTER thread_counter;
    HCOUNTER ws_counter;
    HCOUNTER proc_time_counter;
} PROCESS_HANDLER_INFO;

static double retreive_counter_double_value(HCOUNTER handle_counter)
{
    double result;
    PDH_STATUS phd_status;
    DWORD counter_type;
    PDH_FMT_COUNTERVALUE counter_value;

    if ((phd_status = PdhGetFormattedCounterValue(handle_counter, PDH_FMT_DOUBLE, &counter_type, &counter_value)) != ERROR_SUCCESS)
    {
        LogError("PdhGetFormattedCounterValue failed with status 0x%x.", phd_status);
        result = 0;
    }
    else
    {
        result = counter_value.doubleValue;
    }
    return result;
}

static uint32_t retreive_counter_value(HCOUNTER handle_counter)
{
    uint32_t result;
    PDH_STATUS phd_status;
    DWORD counter_type;
    PDH_FMT_COUNTERVALUE counter_value;

    if ((phd_status = PdhGetFormattedCounterValue(handle_counter, PDH_FMT_LONG, &counter_type, &counter_value)) != ERROR_SUCCESS)
    {
        LogError("PdhGetFormattedCounterValue failed with status 0x%x.", phd_status);
        result = 0;
    }
    else
    {
        result = counter_value.longValue;
    }
    return result;
}

static int get_process_stat(PROCESS_HANDLER_INFO* handler_info, PROCESS_INFO* proc_info)
{
    int result;
    PDH_STATUS phd_status;

    phd_status = PdhCollectQueryData(handler_info->query_handle);
    if (phd_status != ERROR_SUCCESS)
    {
        LogError("Failure opening query 0x%x", phd_status);
        result = __FAILURE__;
    }
    else
    {
        proc_info->handle_cnt = retreive_counter_value(handler_info->handle_counter);
        proc_info->memory_size = retreive_counter_value(handler_info->ws_counter);
        proc_info->num_threads = retreive_counter_value(handler_info->thread_counter);
        proc_info->cpu_load = retreive_counter_double_value(handler_info->proc_time_counter);
        result = 0;
    }
    return result;
}

static int open_counter_query(PROCESS_HANDLER_INFO* handler_info)
{
    int result;
    PDH_STATUS phd_status;
    phd_status = PdhOpenQuery(NULL, 0, &handler_info->query_handle);
    if (phd_status != ERROR_SUCCESS)
    {
        LogError("Failure opening query 0x%x", phd_status);
        result = __FAILURE__;
    }
    else
    {
        char counter_path[64];

        if (sprintf(counter_path, HANDLE_COUNT_COUNTER_FMT, handler_info->process_name) == 0 ||
            ((phd_status = PdhAddCounter(handler_info->query_handle, counter_path, 0, &handler_info->handle_counter)) != ERROR_SUCCESS))
        {
            LogError("Failure adding query handle counter 0x%x", phd_status);
            result = __FAILURE__;
        }
        else if (sprintf(counter_path, THREAD_COUNT_COUNTER_FMT, handler_info->process_name) == 0 ||
            ((phd_status = PdhAddCounter(handler_info->query_handle, counter_path, 0, &handler_info->thread_counter)) != ERROR_SUCCESS))
        {
            LogError("Failure adding query thread counter 0x%x", phd_status);
            result = __FAILURE__;
        }
        else if (sprintf(counter_path, WORKING_SET_COUNTER_FMT, handler_info->process_name) == 0 ||
            ((phd_status = PdhAddCounter(handler_info->query_handle, counter_path, 0, &handler_info->ws_counter)) != ERROR_SUCCESS))
        {
            LogError("Failure adding query working set counter 0x%x", phd_status);
            result = __FAILURE__;
        }
        else if (sprintf(counter_path, PROCESSOR_TIME_COUNTER_FMT, handler_info->process_name) == 0 ||
            ((phd_status = PdhAddCounter(handler_info->query_handle, counter_path, 0, &handler_info->proc_time_counter)) != ERROR_SUCCESS))
        {
            LogError("Failure adding query process time counter 0x%x", phd_status);
            result = __FAILURE__;
        }
        else if ((phd_status = PdhCollectQueryData(handler_info->query_handle)) != ERROR_SUCCESS)
        {
            LogError("Failure intial PdhCollectQueryData 0x%x", phd_status);
            result = __FAILURE__;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

PROCESS_HANDLER_HANDLE process_handler_create(const char* process_path, SDK_TYPE sdk_type, PROCESS_END_CB process_end_cb, void* user_cb)
{
    PROCESS_HANDLER_INFO* result;
    if (process_path == NULL)
    {
        LogError("Invalid argument process path is NULL");
        result = NULL;
    }
    else if ((result = (PROCESS_HANDLER_INFO*)malloc(sizeof(PROCESS_HANDLER_INFO))) != NULL)
    {
        memset(result, 0, sizeof(PROCESS_HANDLER_INFO));
        if (mallocAndStrcpy_s(&result->process_filename, process_path) != 0)
        {
            LogError("Failure allocating process filename");
            free(result);
            result = NULL;
        }
        else if (mallocAndStrcpy_s(&result->process_name, "test_app") != 0)
        {
            LogError("Failure allocating process filename");
            free(result->process_filename);
            free(result);
            result = NULL;
        }
        else
        {
            result->process_end_cb = process_end_cb;
            result->user_cb = user_cb;
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
        result->proc_info.dwProcessId = process_id;
        result->proc_info.hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, result->proc_info.dwProcessId);
        if (result->proc_info.hProcess == NULL)
        {
            LogError("Failure opening process");
            free(result);
            result = NULL;
        }
        else
        {
            result->process_end_cb = process_end_cb;
            result->user_cb = user_cb;
        }
    }
    return result;
}

void process_handler_destroy(PROCESS_HANDLER_HANDLE handle)
{
    if (handle != NULL)
    {
        CloseHandle(handle->proc_info.hProcess);
        handle->proc_info.hProcess = NULL;
        CloseHandle(handle->proc_info.hThread);
        handle->proc_info.hThread = NULL;
        free(handle->process_filename);
        free(handle->process_name);
        free(handle);
    }
}

int process_handler_start(PROCESS_HANDLER_HANDLE handle, const char* cmdline_args)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid argument Process Handler is NULL");
        result = __LINE__;
    }
    else
    {
        STARTUPINFO si;
        memset(&si, 0, sizeof(STARTUPINFO));
        si.cb = sizeof(si);
        if (!CreateProcessA(handle->process_filename, "", NULL, NULL, FALSE, 0, NULL, NULL, &si, &handle->proc_info))
        {
            LogError("Failed to start the process %d", GetLastError() );
            result = __LINE__;
        }
        else if (open_counter_query(handle) != 0)
        {
            LogError("Failed opening counter query");
            result = __LINE__;
        }
        else
        {
            result = 0;
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
        if (process_handler_is_active(handle))
        {
            (void)TerminateProcess(handle->proc_info.hProcess, 0);
        }
        PdhCloseQuery(handle->query_handle);
        result = 0;
    }
    return result;
}

extern bool process_handler_is_active(PROCESS_HANDLER_HANDLE handle)
{
    bool result;
    if (handle == NULL)
    {
        result = false;
    }
    else
    {
        result = (WaitForSingleObject(handle->proc_info.hProcess, 0) != WAIT_OBJECT_0);
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
                result = __LINE__;
            }
        }
    }
    return result;
}

int process_handler_get_network_info(PROCESS_HANDLER_HANDLE handle, NETWORK_INFO* network_info)
{
    int result;
    result = 0;
    return result;
}

/*uint32_t process_handler_get_memory_used(PROCESS_HANDLER_HANDLE handle)
{
    uint32_t result;
    if (handle == NULL || handle->proc_info.hProcess == NULL)
    {
        result = __LINE__;
    }
    else
    {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
        if (!GetProcessMemoryInfo(handle->proc_info.hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        {
            result = 0;
        }
        else
        {
            printf("\tPageFaultCount: %d\n", pmc.PageFaultCount);
            printf("\tPeakWorkingSetSize: %d\n", pmc.PeakWorkingSetSize);
            printf("\tWorkingSetSize: %d\n", pmc.WorkingSetSize);
            printf("\tQuotaPeakPagedPoolUsage: %d\n", pmc.QuotaPeakPagedPoolUsage);
            printf("\tQuotaPagedPoolUsage: %d\n", pmc.QuotaPagedPoolUsage);
            printf("\tQuotaPeakNonPagedPoolUsage: %d\n", pmc.QuotaPeakNonPagedPoolUsage);
            printf("\tQuotaNonPagedPoolUsage: %d\n", pmc.QuotaNonPagedPoolUsage);
            printf("\tPagefileUsage: %d\n", pmc.PagefileUsage);
            printf("\tPeakPagefileUsage: %d\n", pmc.PeakPagefileUsage);
            result = pmc.PrivateUsage;
        }
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
}*/

