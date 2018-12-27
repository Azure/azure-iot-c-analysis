// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#ifdef WIN32
#include <windows.h>
#else
#define _GNU_SOURCE
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include "azure_c_shared_utility/strings.h"
#endif

#include "health_process_item.h"
#include "binary_handler.h"
#include "parson.h"

#include "azure_c_shared_utility/xlogging.h"

#ifdef WIN32
static const char DIR_SEPARATOR = '\\';
#else
static const char DIR_SEPARATOR = '/';
#endif

static const char* const SIZE_NODE = "size";
static const char* const BINARY_OBJECT_NODE = "diskSize";

typedef struct DATA_SIZE_INFO_TAG
{
    uint64_t data_len;
} DATA_SIZE_INFO;

static uint64_t calculate_exe_size(const char* file_path)
{
    uint64_t result;
#ifdef WIN32
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
#else
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
#endif
    return result;
}

static uint64_t calculate_dir_size(const char* file_dir)
{
    uint64_t result = 0;
#ifdef WIN32
    WIN32_FIND_DATA ffd;

    char full_path[MAX_PATH];
    sprintf(full_path, "%s\\*", file_dir);
    HANDLE find_handle = FindFirstFileA(full_path, &ffd);
    if (find_handle == INVALID_HANDLE_VALUE)
    {
        LogError("Failure opening Findfile %s", file_dir);
        result = 0;
    }
    else
    {
        do
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                sprintf(full_path, "%s/%s", file_dir, ffd.cFileName);
                calculate_dir_size(full_path);
            }
            else
            {
                LARGE_INTEGER file_size;
                file_size.LowPart = ffd.nFileSizeLow;
                file_size.HighPart = ffd.nFileSizeHigh;
                result += file_size.QuadPart;
            }
        } while (FindNextFile(find_handle, &ffd) != 0);
        CloseHandle(find_handle);
    }
#else
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
            STRING_HANDLE full_path = STRING_construct_sprintf("%s/%s", file_dir, sdir->d_name);
            if (full_path == NULL)
            {
                LogError("Failure constructing full path");
                result = 0;
                break;
            }
            else
            {
                if (sdir->d_type == DT_DIR)
                {
                    if (sdir->d_name[0] != '.')
                    {
                        result += calculate_dir_size(STRING_c_str(full_path));
                    }
                }
                else if (sdir->d_type == DT_REG || sdir->d_type == DT_LNK)
                {
                    result += calculate_exe_size(STRING_c_str(full_path));
                }
                STRING_delete(full_path);
            }
        }
    }
#endif
    return result;
}

static int construct_binary_json(HEALTH_ITEM_HANDLE handle, JSON_Object* object_node)
{
    int result;
    if (handle == NULL || object_node == NULL)
    {
        LogError("Failure initializing device info object");
        result = __FAILURE__;
    }
    else
    {
        DATA_SIZE_INFO* data_size = (DATA_SIZE_INFO*)handle;

        JSON_Value* value_node = json_value_init_object();
        if (value_node == NULL)
        {
            LogError("Failure initializing device info object");
            result = __FAILURE__;
        }
        else
        {
            JSON_Object* dev_info_obj = json_value_get_object(value_node);

            if (json_object_set_number(dev_info_obj, SIZE_NODE, (double)data_size->data_len) != JSONSuccess)
            {
                LogError("Failure setting cpu count node");
                result = __FAILURE__;
            }
            else if (json_object_set_value(object_node, BINARY_OBJECT_NODE, value_node) != JSONSuccess)
            {
                LogError("Failure setting json object");
                result = __FAILURE__;
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}

HEALTH_ITEM_HANDLE binary_handler_get_size(const char* file_path, SDK_TYPE type)
{
    DATA_SIZE_INFO* result;
    if (file_path == NULL)
    {
        LogError("Invalid argument file_path is NULL");
        result = NULL;
    }
    else if ((result = (DATA_SIZE_INFO*)malloc(sizeof(DATA_SIZE_INFO))) == NULL)
    {
        LogError("Unable to allocate Data size");
    }
    else
    {
        if (type == SDK_TYPE_C)
        {
            result->data_len = calculate_exe_size(file_path);
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
            result->data_len = calculate_dir_size(dir_path);
        }
        else
        {
            result->data_len = 0;
        }
    }
    return (HEALTH_ITEM_HANDLE)result;
}

void binary_handler_destroy(HEALTH_ITEM_HANDLE handle)
{
    if (handle != NULL)
    {
        DATA_SIZE_INFO* data_size = (DATA_SIZE_INFO*)handle;
        free(data_size);
    }
}

HEALTH_REPORTER_CONSTRUCT_JSON binary_handler_get_json_cb(void)
{
    return construct_binary_json;
}
