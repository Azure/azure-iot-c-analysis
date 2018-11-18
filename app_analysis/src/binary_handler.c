// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>

#include "binary_handler.h"

#include "azure_c_shared_utility/xlogging.h"

static uint32_t calculate_exe_size(const char* file_path)
{
    uint32_t result;
    FILE* target_file = fopen(file_path, "rb");
    if (target_file == NULL)
    {
        // If the file isn't there then don't report on it and just return
        LogError("Failure opening file %s", file_path);
        result = 0;
    }
    else
    {
        fseek(target_file, 0, SEEK_END);
        result = ftell(target_file);
        fclose(target_file);
    }
    return result;
}

uint32_t binary_handler_get_size(const char* file_path, SDK_TYPE type)
{
    uint32_t result;
    if (file_path == NULL)
    {
        LogError("Invalid argument file_path is NULL");
        result = 0;
    }
    else
    {
        if (type == SDK_TYPE_C)
        {
            result = calculate_exe_size(file_path);
        }
        else
        {
            result = 0;
        }
    }
    return result;
}
