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

typedef struct PROCESS_INFO_TAG
{
    uint32_t num_threads;
    uint32_t memory_size;
} PROCESS_INFO;

typedef struct PROCESS_HANDLER_INFO_TAG* PROCESS_HANDLER_HANDLE;

typedef void(*PROCESS_END_CB)(void* user_cb);

extern PROCESS_HANDLER_HANDLE process_handler_create(const char* process_path, PROCESS_END_CB process_end_cb, void* user_cb);
extern PROCESS_HANDLER_HANDLE process_handler_create_by_pid(int process_id, PROCESS_END_CB process_end_cb, void* user_cb);
extern void process_handler_destroy(PROCESS_HANDLER_HANDLE handle);


extern int process_handler_start(PROCESS_HANDLER_HANDLE handle, const char* cmdline_args);
extern int process_handler_end(PROCESS_HANDLER_HANDLE handle);

extern bool process_handler_is_active(PROCESS_HANDLER_HANDLE handle);
extern int process_handler_get_process_info(PROCESS_HANDLER_HANDLE handle, PROCESS_INFO* proc_info);

//extern uint32_t process_handler_get_bin_size(PROCESS_HANDLER_HANDLE handle);
extern uint32_t process_handler_get_memory_used(PROCESS_HANDLER_HANDLE handle);
extern uint32_t process_handler_get_threads(PROCESS_HANDLER_HANDLE handle);

#endif // PROCESS_HANDLER_H
