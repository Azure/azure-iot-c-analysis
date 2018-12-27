// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BINARY_HANDLER_H
#define BINARY_HANDLER_H

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

#include "app_analysis_const.h"

extern HEALTH_ITEM_HANDLE binary_handler_get_size(const char* file_path, SDK_TYPE type);
extern void binary_handler_destroy(HEALTH_ITEM_HANDLE handle);
extern HEALTH_REPORTER_CONSTRUCT_JSON binary_handler_get_json_cb(void);

#endif // BINARY_HANDLER_H
