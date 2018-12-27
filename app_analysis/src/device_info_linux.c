// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "device_info.h"
#include "app_analysis_const.h"
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
        device_info->cpu_count = (uint8_t)sysconf(_SC_NPROCESSORS_ONLN);
        device_info->memory_amt = (uint64_t)sysconf(_SC_AVPHYS_PAGES);
        result = 0;
    }
    return result;
}
