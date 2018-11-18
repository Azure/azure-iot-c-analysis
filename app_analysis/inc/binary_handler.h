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

typedef enum SDK_TYPE_TAG
{
    SDK_TYPE_C,
    SDK_TYPE_CSHARP,
    SDK_TYPE_NODE,
    SDK_TYPE_JAVA,
    SDK_TYPE_PYTHON
} SDK_TYPE;

extern uint32_t binary_handler_get_size(const char* file_path, SDK_TYPE type);

#endif // BINARY_HANDLER_H
