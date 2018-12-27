// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <windows.h>

#include "app_analysis_const.h"
#include "device_info.h"
#include "azure_c_shared_utility/xlogging.h"


int device_info_load(DEVICE_HEALTH_INFO* device_info)
{
    int result;
    if (device_info == NULL)
    {
        LogError("Failure initializing device info object");
        result = __FAILURE__;
    }
    else
    {
        MEMORYSTATUSEX mem_status;
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        device_info->cpu_count = (uint8_t)sys_info.dwNumberOfProcessors;


        mem_status.dwLength = sizeof(mem_status);
        GlobalMemoryStatusEx(&mem_status);
        device_info->memory_amt = (uint64_t)mem_status.ullTotalPhys;

        result = 0;
    }
    return result;
}
