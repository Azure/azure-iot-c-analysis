// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "hash_table.h"

#include "azure_c_shared_utility/xlogging.h"

typedef struct HASH_NODE_TAG
{
    uint32_t key;
    void* hash_value;
    struct HASH_NODE_TAG* next;
} HASH_NODE;

typedef struct HASH_TABLE_INFO_TAG
{
    HASH_TABLE_REMOVE_ITEM rm_item_cb;
    void* user_ctx;
    uint32_t table_size;
    HASH_NODE** hash_list;  // An array of Nodes
} HASH_TABLE_INFO;

#define     DEFAULT_INIT_ALLOC_SIZE     32

static hash_table_key hash_function(const HASH_TABLE_INFO* table_info, uint32_t key)
{
    // Look up good hash function
    return key % table_info->table_size;
}

static void clear_table(HASH_TABLE_INFO* table_info)
{
    for (size_t index = 0; index < table_info->table_size; index++)
    {
        HASH_NODE* list_pos = table_info->hash_list[index];
        HASH_NODE* temp = list_pos;
        while (temp)
        {
            if (table_info->rm_item_cb != NULL)
            {
                table_info->rm_item_cb(temp->key, temp->hash_value, table_info->user_ctx);
            }
            HASH_NODE* free_item = temp;
            temp = temp->next;
            free(free_item);
        }
    }
}

HASH_TABLE_HANDLE hash_table_create(size_t init_size, HASH_TABLE_REMOVE_ITEM rm_item_cb, void* user_ctx)
{
    HASH_TABLE_INFO* result;
    if ((result = (HASH_TABLE_INFO*)malloc(sizeof(HASH_TABLE_INFO))) == NULL)
    {
        LogError("Failure allocating hash table information");
    }
    else
    {
        result->table_size = init_size;
        if (result->table_size == 0)
        {
            result->table_size = DEFAULT_INIT_ALLOC_SIZE;
        }

        size_t total_size = sizeof(HASH_NODE)*result->table_size;
        if ((result->hash_list = (HASH_NODE**)malloc(total_size)) == NULL)
        {
            LogError("Failure allocating hash table node information");
            free(result);
        }
        else
        {
            memset(result->hash_list, 0, total_size);
            result->rm_item_cb = rm_item_cb;
            result->user_ctx = user_ctx;
        }
    }
    return result;
}

void hash_table_destroy(HASH_TABLE_HANDLE handle)
{
    if (handle != NULL)
    {
        clear_table(handle);
        free(handle->hash_list);
        free(handle);
    }
}

void hash_table_clear(HASH_TABLE_HANDLE handle)
{
    if (handle != NULL)
    {
        clear_table(handle);
    }
}

int hash_table_add_item(HASH_TABLE_HANDLE handle, hash_table_key key, void* item)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid argument handle is NULL");
        result = __FAILURE__;
    }
    // Check for realloc
    else
    {
        bool duplicate_item = false;
        hash_table_key pos = hash_function(handle, key);

        // The position of the hash function
        HASH_NODE* list_pos = handle->hash_list[pos];

        HASH_NODE* temp = list_pos;
        // See if we need to use the link list
        while (temp)
        {
            // If the keys are the same then replace the item
            if (temp->key == key) {
                temp->hash_value = item;
                duplicate_item = true;
            }
            temp = temp->next;
        }
        result = 0;

        if (!duplicate_item)
        {
            HASH_NODE* new_hash_node = (HASH_NODE*)malloc(sizeof(HASH_NODE));
            if (new_hash_node == NULL)
            {
                LogError("Failure allocating Hash node");
                result = __FAILURE__;
            }
            else
            {
                new_hash_node->key = key;
                new_hash_node->hash_value = item;
                new_hash_node->next = NULL;
                if (temp == NULL)
                {
                    handle->hash_list[pos] = new_hash_node;
                }
                else
                {
                    temp->next = new_hash_node;
                }
            }
        }
    }
    return result;
}

void* hash_table_lookup(HASH_TABLE_HANDLE handle, hash_table_key key)
{
    void* result = NULL;
    if (handle == NULL)
    {
        LogError("Invalid argument handle is NULL");
    }
    else
    {
        hash_table_key pos = hash_function(handle, key);
        HASH_NODE* list_pos = handle->hash_list[pos];
        HASH_NODE* temp_node = list_pos;
        while (temp_node)
        {
            if (temp_node->key == key)
            {
                result = temp_node->hash_value;
                break;
            }
        }
    }
    return result;
}

