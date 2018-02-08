// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MEMORY_ANALYSIS_H
#define MEMORY_ANALYSIS_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif

static const char* MQTT_PROTOCOL_NAME = "MQTT PROTOCOL";
static const char* MQTT_WS_PROTOCOL_NAME = "MQTT WS PROTOCOL";
static const char* HTTP_PROTOCOL_NAME = "HTTP PROTOCOL";
static const char* AMQP_PROTOCOL_NAME = "AMQP PROTOCOL";
static const char* AMQP_WS_PROTOCOL_NAME = "AMQP WS PROTOCOL";

#ifdef WIN32
    static const char* OS_NAME = "Windows";
#else
    static const char* OS_NAME = "Linux";
#endif

    typedef enum PROTOCOL_TYPE_TAG
    {
        PROTOCOL_UNKNOWN,
        PROTOCOL_MQTT,
        PROTOCOL_MQTT_WS,
        PROTOCOL_HTTP,
        PROTOCOL_AMQP,
        PROTOCOL_AMQP_WS
    } PROTOCOL_TYPE;

    typedef enum OPERATION_TYPE_TAG
    {
        OPERATION_TELEMETRY_LL,
        OPERATION_TELEMETRY_UL,
        OPERATION_C2D_LL,
        OPERATION_C2D_UL,
        OPERATION_BINARY_SIZE_LL,
        OPERATION_BINARY_SIZE_UL,
        OPERATION_METHODS,
        OPERATION_TWIN,
        OPERATION_PROVISIONING
    } OPERATION_TYPE;
    
    typedef struct MEM_ANALYSIS_INFO_TAG
    {
        const char* iothub_version;
        PROTOCOL_TYPE iothub_protocol;
        OPERATION_TYPE operation_type;
        size_t msg_sent;
    } MEM_ANALYSIS_INFO;

    typedef struct BINARY_INFO_TAG
    {
        const char* iothub_version;
        PROTOCOL_TYPE iothub_protocol;
        OPERATION_TYPE operation_type;
        const char* cmake_dir;
        long binary_size;
    } BINARY_INFO;

    extern void record_memory_usage(const MEM_ANALYSIS_INFO* iot_mem_info);
    extern void report_binary_sizes(const BINARY_INFO* bin_info);

#ifdef __cplusplus
}
#endif


#endif  /* MEMORY_ANALYSIS_H */
