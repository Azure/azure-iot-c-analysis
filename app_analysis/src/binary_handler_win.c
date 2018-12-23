// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "binary_handler.h"

#include "azure_c_shared_utility/xlogging.h"

#ifdef WIN32
static const char DIR_SEPARATOR = '\\';
#else
static const char DIR_SEPARATOR = '/';
#endif

static int calculate_size(const char* fpath, const struct stat* sb, int type_flag)
{
    return 0;
}

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

static uint64_t calculate_dir_size(const char* file_dir)
{
    uint64_t result = 0;
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
        else if (type == SDK_TYPE_NODE)
        {
            char dir_path[128];
            size_t path_count = strlen(file_path);
            memset(dir_path, 0, 128);

            for (size_t index = path_count-1; index > 0; index--)
            {
                if (file_path[index] == DIR_SEPARATOR)
                {
                    strncpy(dir_path, file_path, index);
                    break;
                }
            }
            uint64_t cal = calculate_dir_size(dir_path);
            result = 0;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}
