// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SDK_ANALYSIS_MSG_H
#define SDK_ANALYSIS_MSG_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdbool>
extern "C" {
#else
#include <stddef.h>
#include <stdbool.h>
#endif

#include "mem_reporter.h"

extern int initiate_lower_level_operation(const char* conn_string, PROTOCOL_TYPE protocol, size_t num_msgs_to_recv, bool use_byte_array_msg);
extern int initiate_upper_level_operation(const char* conn_string, PROTOCOL_TYPE protocol, size_t num_msgs_to_recv, bool use_byte_array_msg);

#ifdef __cplusplus
}
#endif


#endif  /* SDK_ANALYSIS_MSG_H */
