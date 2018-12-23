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

static uint64_t calculate_exe_size(const char* file_path)
{
    uint64_t result;
    struct stat stat_info;
    if (lstat(file_path, &stat_info) != 0)
    {
        LogError("Failure retrieving file information %s", file_path);
        result = 0;
    }
    else
    {
        result = stat_info.st_size;
    }
    return result;
}

static uint64_t calculate_dir_size(const char* file_dir)
{
    uint64_t result = 0;
    DIR* dptr;

    if ((dptr = opendir(file_dir)) == NULL)
    {
        LogError("Failured opening directory");
        result = 0;
    }
    else
    {
        struct dirent* sdir;
        while (sdir = readdir(dptr))
        {
            char full_path[256];
            sprintf(full_path, "%s/%s", file_dir, sdir->d_name);
            if (sdir->d_type == DT_DIR)
            {
                if (sdir->d_name[0] != '.')
                {
                    result += calculate_dir_size(full_path);
                }
            }
            else if (sdir->d_type == DT_REG || sdir->d_type == DT_LNK)
            {
                result += calculate_exe_size(full_path);
            }
        }
    }
    return result;
}

uint64_t binary_handler_get_size(const char* file_path, SDK_TYPE type)
{
    uint64_t result;
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
            result = calculate_dir_size(dir_path);
        }
        else
        {
            result = 0;
        }
    }
    return result;
}
