// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <Windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include "process_handler.h"

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/xlogging.h"

typedef struct PROCESS_HANDLER_INFO_TAG
{
    char* process_filename;
    PROCESS_INFORMATION proc_info;
    PROCESS_END_CB process_end_cb;
    void* user_cb;
} PROCESS_HANDLER_INFO;

static int get_process_stat(PROCESS_HANDLER_HANDLE handle, PROCESS_INFO* proc_info)
{
    int result;

    APP_MEMORY_INFORMATION app_mem_info;
    if (GetProcessInformation(handle->proc_info.hProcess, ProcessAppMemoryInfo, &app_mem_info, sizeof(app_mem_info)))
    {
        proc_info->memory_size = (uint32_t)app_mem_info.TotalCommitUsage;
    }

    DWORD handle_cnt;
    if (GetProcessHandleCount(handle->proc_info.hProcess, &handle_cnt))
    {
        proc_info->handle_cnt = handle_cnt;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, handle->proc_info.dwProcessId);
    if (snapshot == NULL)
    {
        LogError("Failed creating snapshot of system");
        result = __LINE__;
    }
    else
    {
        // Get the thread information
        PROCESSENTRY32 proc_entry;
        proc_entry.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(snapshot, &proc_entry))
        {
            LogError("Failed retrieving snapshot");
            result = __LINE__;
        }
        else
        {
            do
            {
                if (proc_entry.th32ProcessID == handle->proc_info.dwProcessId)
                {
                    proc_info->num_threads = proc_entry.cntThreads;
                }
            } while (Process32Next(snapshot, &proc_entry));
            result = 0;
        }
        CloseHandle(snapshot);


        /*PROCESS_MEMORY_COUNTERS_EX pmc;
        pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
        if (!GetProcessMemoryInfo(handle->proc_info.hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        {
            LogError("Failed retrieving memory info from process %d", GetLastError());
            result = __LINE__;
        }
        else
        {
            /*printf("\tPageFaultCount: %d\n", pmc.PageFaultCount);
            printf("\tPeakWorkingSetSize: %d\n", pmc.PeakWorkingSetSize);
            printf("\tWorkingSetSize: %d\n", pmc.WorkingSetSize);
            printf("\tQuotaPeakPagedPoolUsage: %d\n", pmc.QuotaPeakPagedPoolUsage);
            printf("\tQuotaPagedPoolUsage: %d\n", pmc.QuotaPagedPoolUsage);
            printf("\tQuotaPeakNonPagedPoolUsage: %d\n", pmc.QuotaPeakNonPagedPoolUsage);
            printf("\tQuotaNonPagedPoolUsage: %d\n", pmc.QuotaNonPagedPoolUsage);
            printf("\tPagefileUsage: %d\n", pmc.PagefileUsage);
            printf("\tPeakPagefileUsage: %d\n", pmc.PeakPagefileUsage);*/
            /*proc_info->memory_size = pmc.WorkingSetSize;
        }*/
    }
    return result;
}

PROCESS_HANDLER_HANDLE process_handler_create(const char* process_path, PROCESS_END_CB process_end_cb, void* user_cb)
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
        else
        {
            printf("Is active %s", process_handler_is_active(handle) ? "true" : "false");

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
        if (!TerminateProcess(handle->proc_info.hProcess, 0))
        {
            result = __LINE__;
        }
        else
        {
            result = 0;
        }
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
        if (get_process_stat(handle, proc_info) == 0)
        {
            result = 0;
        }
        else
        {
            result = 0;
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

