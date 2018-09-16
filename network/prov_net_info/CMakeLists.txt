#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(prov_net_info_c_files
    prov_net_info.c
    ../mem_analytics.c
    ../../mem_reporter.c
    ../../certs/certs.c
)

set(prov_net_info_h_files
    prov_net_info.h
    ../../mem_reporter.h
    ../../certs/certs.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

add_definitions(-DUSE_NETWORKING -DPROV_CLIENT)
add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC -DGB_DEBUG_NETWORK -DGB_MEASURE_NETWORK_FOR_THIS)

include_directories(. .. ../..)
include_directories(${SDK_INCLUDE_DIRS})

add_executable(prov_net_info ${prov_net_info_c_files} ${prov_net_info_h_files})
target_link_libraries(prov_net_info 
    prov_device_client
    prov_mqtt_transport
    prov_mqtt_ws_transport
    prov_amqp_ws_transport 
    prov_amqp_transport
    prov_http_transport
    iothub_service_client
    uamqp
    umqtt
    aziotsharedutil
)

if(WIN32)
    target_link_libraries(prov_net_info ws2_32 rpcrt4 ncrypt winhttp secur32 crypt32)
else()
    target_link_libraries(prov_net_info m)
endif()
