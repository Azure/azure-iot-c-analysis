// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MEMORY_ANALYSIS_H
#define MEMORY_ANALYSIS_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stdbool.h>
#include <stddef.h>
#endif

static const char* MQTT_PROTOCOL_NAME = "MQTT PROTOCOL";
static const char* MQTT_WS_PROTOCOL_NAME = "MQTT WS PROTOCOL";
static const char* HTTP_PROTOCOL_NAME = "HTTP PROTOCOL";
static const char* AMQP_PROTOCOL_NAME = "AMQP PROTOCOL";
static const char* AMQP_WS_PROTOCOL_NAME = "AMQP WS PROTOCOL";

typedef struct REPORT_INFO_TAG* REPORT_HANDLE;

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

    typedef enum FEATURE_TYPE_TAG
    {
        FEATURE_TELEMETRY_LL,
        FEATURE_TELEMETRY_UL,
        FEATURE_C2D_LL,
        FEATURE_C2D_UL,
        FEATURE_METHODS_LL,
        FEATURE_METHODS_UL,
        FEATURE_TWIN_LL,
        FEATURE_TWIN_UL,
        FEATURE_PROVISIONING_LL,
        FEATURE_PROVISIONING_UL
    } FEATURE_TYPE;

    typedef enum OPERATION_TYPE_TAG
    {
        OPERATION_NETWORK,
        OPERATION_MEMORY,
        OPERATION_BINARY_SIZE,
    } OPERATION_TYPE;

    typedef enum REPORTER_TYPE_TAG
    {
        REPORTER_TYPE_JSON,
        REPORTER_TYPE_CVS,
        REPORTER_TYPE_MD,
    } REPORTER_TYPE;

    typedef struct CONNECTION_INFO_TAG
    {
        char* device_conn_string;
        char* scope_id;
    } CONNECTION_INFO;

    typedef struct MEM_ANALYSIS_INFO_TAG
    {
        const char* iothub_version;
        PROTOCOL_TYPE iothub_protocol;
        OPERATION_TYPE operation_type;
        FEATURE_TYPE feature_type;
        size_t msg_sent;
    } MEM_ANALYSIS_INFO;

    typedef struct BINARY_INFO_TAG
    {
        const char* iothub_version;
        PROTOCOL_TYPE iothub_protocol;
        OPERATION_TYPE operation_type;
        FEATURE_TYPE feature_type;
        const char* cmake_dir;
        long binary_size;
        const char* output_file;
        bool skip_ul;
    } BINARY_INFO;

    extern REPORT_HANDLE report_initialize(REPORTER_TYPE type);
    extern void report_deinitialize(REPORT_HANDLE handle);
    
    extern void report_memory_usage(REPORT_HANDLE handle, const MEM_ANALYSIS_INFO* iot_mem_info);
    extern void report_binary_sizes(REPORT_HANDLE handle, const BINARY_INFO* bin_info);
    extern void report_network_usage(REPORT_HANDLE handle, const MEM_ANALYSIS_INFO* iot_mem_info);

    extern bool report_write(REPORT_HANDLE handle);

#ifdef __cplusplus
}
#endif


#endif  /* MEMORY_ANALYSIS_H */
