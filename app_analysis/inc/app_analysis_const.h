// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef APP_ANALYSIS_CONST_H
#define APP_ANALYSIS_CONST_H

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

#include "health_process_item.h"

#ifdef WIN32
    static const char* OS_NAME = "Windows";
#else
    static const char* OS_NAME = "Linux";
#endif

typedef void* HEALTH_ITEM_HANDLE;

typedef int(*HEALTH_REPORTER_CONSTRUCT_JSON)(HEALTH_ITEM_HANDLE handle, JSON_Object* object_node);

typedef enum PROTOCOL_TYPE_TAG
{
    PROTOCOL_UNKNOWN,
    PROTOCOL_MQTT,
    PROTOCOL_MQTT_WS,
    PROTOCOL_HTTP,
    PROTOCOL_AMQP,
    PROTOCOL_AMQP_WS
} PROTOCOL_TYPE;

typedef enum REPORTER_TYPE_TAG
{
    REPORTER_TYPE_JSON,
    REPORTER_TYPE_CSV,
    REPORTER_TYPE_MD
} REPORTER_TYPE;

typedef enum SDK_TYPE_TAG
{
    SDK_TYPE_UNKNOWN,
    SDK_TYPE_C,
    SDK_TYPE_CSHARP,
    SDK_TYPE_JAVA,
    SDK_TYPE_NODE,
    SDK_TYPE_PYTHON
} SDK_TYPE;

typedef struct PROCESS_INFO_TAG
{
    uint32_t num_threads;
    uint32_t memory_size;
    uint32_t virtual_mem_size;
    uint32_t handle_cnt;
    double cpu_load;
} PROCESS_INFO;

typedef struct NETWORK_INFO_TAG
{
    uint32_t bytes_recv;
    uint32_t packets_recv;
    uint32_t bytes_transmit;
    uint32_t packets_transmit;
    uint32_t msg_size;
} NETWORK_INFO;

typedef struct EXECUTABLE_INFO_TAG
{
    HEALTH_ITEM_HANDLE app_size;
} EXECUTABLE_INFO;

typedef struct DEVICE_HEALTH_TAG
{
    uint8_t cpu_count;
    uint64_t memory_amt;
} DEVICE_HEALTH_INFO;

#endif // APP_ANALYSIS_CONST_H
