// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PROCESS_HANDLER_H
#define PROCESS_HANDLER_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C" {
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

#include "app_analysis_const.h"

typedef struct PROCESS_HANDLER_INFO_TAG* PROCESS_HANDLER_HANDLE;

typedef void(*PROCESS_END_CB)(void* user_cb);

extern PROCESS_HANDLER_HANDLE process_handler_create(const char* process_path, PROCESS_END_CB process_end_cb, void* user_cb);
extern PROCESS_HANDLER_HANDLE process_handler_create_by_pid(int process_id, PROCESS_END_CB process_end_cb, void* user_cb);
extern void process_handler_destroy(PROCESS_HANDLER_HANDLE handle);


extern int process_handler_start(PROCESS_HANDLER_HANDLE handle, const char* cmdline_args);
extern int process_handler_end(PROCESS_HANDLER_HANDLE handle);

extern bool process_handler_is_active(PROCESS_HANDLER_HANDLE handle);
extern int process_handler_get_process_info(PROCESS_HANDLER_HANDLE handle, PROCESS_INFO* proc_info);
extern int process_handler_get_network_info(PROCESS_HANDLER_HANDLE handle, NETWORK_INFO* network_info);

#endif // PROCESS_HANDLER_H
