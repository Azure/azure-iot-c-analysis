// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DEV_HEALTH_REPORTER_H
#define DEV_HEALTH_REPORTER_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stdbool.h>
#include <stddef.h>
#endif

#include "azure_c_shared_utility/umock_c_prod.h"

#include "app_analysis_const.h"
#include "parson.h"

typedef struct REPORT_INFO_TAG* HEALTH_REPORTER_HANDLE;

typedef struct DEVICE_HEALTH_TAG
{
    uint8_t cpu_count;
    uint32_t memory_amt;
} DEVICE_HEALTH_INFO;


typedef JSON_Object(*HEALTH_REPORTER_CONSTRUCT_JSON)(void);


    MOCKABLE_FUNCTION(, HEALTH_REPORTER_HANDLE, health_reporter_init, REPORTER_TYPE, rpt_type, SDK_TYPE, sdk_type, const DEVICE_HEALTH_INFO*, device_info);
    MOCKABLE_FUNCTION(, void, health_reporter_deinit, HEALTH_REPORTER_HANDLE, handle);
    
    MOCKABLE_FUNCTION(, int, health_reporter_register_health_item, HEALTH_REPORTER_HANDLE, handle, uint32_t, item_index, HEALTH_REPORTER_CONSTRUCT_JSON, construct_json);

    MOCKABLE_FUNCTION(, int, health_reporter_process_health_run, HEALTH_REPORTER_HANDLE, handle);

    extern void report_memory_usage(HEALTH_REPORTER_HANDLE handle, const char* description, const PROCESS_INFO* process_info);
    extern void report_binary_sizes(HEALTH_REPORTER_HANDLE handle, const char* description, const EXECUTABLE_INFO* exe_info);
    extern void report_network_usage(HEALTH_REPORTER_HANDLE handle, const char* description, const NETWORK_INFO* network_info);

    extern bool report_write(HEALTH_REPORTER_HANDLE handle, const char* output_file, const char* conn_string);

#ifdef __cplusplus
}
#endif


#endif  /* DEV_HEALTH_REPORTER_H */
