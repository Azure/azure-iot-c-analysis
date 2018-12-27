// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

    #include "azure_c_shared_utility/umock_c_prod.h"

    typedef uint32_t hash_table_key;

    typedef struct HASH_TABLE_INFO_TAG* HASH_TABLE_HANDLE;
    typedef struct HASH_ITERATOR_TAG* HASH_ITERATOR_HANDLE;

    typedef void(*HASH_TABLE_REMOVE_ITEM)(hash_table_key key, void* item, void* user_ctx);

    MOCKABLE_FUNCTION(, HASH_TABLE_HANDLE, hash_table_create, size_t, init_size, HASH_TABLE_REMOVE_ITEM, rm_item_cb, void*, user_ctx);
    MOCKABLE_FUNCTION(, void, hash_table_destroy, HASH_TABLE_HANDLE, handle);
    MOCKABLE_FUNCTION(, void, hash_table_clear, HASH_TABLE_HANDLE, handle);

    MOCKABLE_FUNCTION(, int, hash_table_add_item, HASH_TABLE_HANDLE, handle, hash_table_key, key, void*, item);
    MOCKABLE_FUNCTION(, void*, hash_table_lookup, HASH_TABLE_HANDLE, handle, hash_table_key, key);

    MOCKABLE_FUNCTION(, HASH_ITERATOR_HANDLE, hash_table_get_first_item, HASH_TABLE_HANDLE, handle, hash_table_key*, key, void*, item);
    MOCKABLE_FUNCTION(, int, hash_table_get_next_item, HASH_ITERATOR_HANDLE, index_handle, hash_table_key*, key, void*, item);
    MOCKABLE_FUNCTION(, void, hash_table_destroy_iterator, HASH_ITERATOR_HANDLE, index_handle);

#endif // HASH_TABLE_H
