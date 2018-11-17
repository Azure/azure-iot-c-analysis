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

typedef void* PROCESS_HANDLER_HANDLE;
//typedef struct PROCESS_HANDLER_INFO* PROCESS_HANDLER_HANDLE;

extern PROCESS_HANDLER_HANDLE process_handler_create(const char* process_path);
extern void process_handler_destroy(PROCESS_HANDLER_HANDLE handle);

extern int process_handler_start(PROCESS_HANDLER_HANDLE handle, const char* cmdline_args);
extern uint32_t process_handler_get_bin_size(PROCESS_HANDLER_HANDLE handle);
extern uint32_t process_handler_get_memory_used(PROCESS_HANDLER_HANDLE handle);
extern uint32_t process_handler_get_threads(PROCESS_HANDLER_HANDLE handle);

#endif // PROCESS_HANDLER_H
