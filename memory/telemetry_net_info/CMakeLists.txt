#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(network_info_c_files
    network_info.c
    ../mem_analytics.c
    ../../mem_reporter.c
    ../../certs/certs.c
)

set(network_info_h_files
    network_info.h
    ../../mem_reporter.h
    ../../certs/certs.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

add_definitions(-DUSE_NETWORKING)
add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC -DGB_DEBUG_NETWORK -DGB_MEASURE_NETWORK_FOR_THIS)

include_directories(. .. ../..)
include_directories(${SDK_INCLUDE_DIRS})

add_executable(telemetry_network_info ${network_info_c_files} ${network_info_h_files})
target_link_libraries(telemetry_network_info 
    iothub_client
    iothub_client_mqtt_transport
    iothub_client_mqtt_ws_transport
    iothub_client_amqp_transport
    iothub_client_amqp_ws_transport
    iothub_client_http_transport
    iothub_service_client
    uamqp
    umqtt
    aziotsharedutil
)

if(WIN32)
    target_link_libraries(telemetry_network_info ws2_32 rpcrt4 ncrypt winhttp secur32 crypt32)
else()
    target_link_libraries(telemetry_network_info m)
endif()
