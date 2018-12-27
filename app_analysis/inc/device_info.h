// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

#include "app_analysis_const.h"

extern int device_info_load(DEVICE_HEALTH_INFO* device_info);

#endif // DEVICE_INFO_H
