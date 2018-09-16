#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(network_info_c_files
    network_info.c
    ../network_analytics.c
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

if (${use_mqtt})
    add_definitions(-DUSE_MQTT)
endif()
if (${use_amqp})
    add_definitions(-DUSE_AMQP)
endif()
if (${use_http})
    add_definitions(-DUSE_HTTP)
endif()
if (${use_prov_client})
    add_definitions(-DUSE_PROVISIONING_CLIENT)
endif()

add_definitions(-DUSE_NETWORKING -DIOTHUB_CLIENT)
add_definitions(-DGB_DEBUG_NETWORK -DGB_MEASURE_NETWORK_FOR_THIS)

include_directories(${CMAKE_CURRENT_LIST_DIR} ${REPORTER_DIR} ${REPORTER_DIR}/deps/parson)
include_directories(${SDK_INCLUDE_DIRS})

add_executable(telemetry_net_info ${network_info_c_files} ${network_info_h_files})
target_link_libraries(telemetry_net_info 
    iothub_client
    aziotsharedutil
)
if (${use_mqtt})
    target_link_libraries(telemetry_net_info 
        iothub_client_mqtt_transport
        iothub_client_mqtt_ws_transport
        umqtt
    )
endif()
if (${use_amqp})
    target_link_libraries(telemetry_net_info 
        iothub_client_amqp_transport
        iothub_client_amqp_ws_transport
        uamqp
    )
endif()
if (${use_http})
    target_link_libraries(telemetry_net_info 
        iothub_service_client
        iothub_client_http_transport
    )
endif()

if(WIN32)
    target_link_libraries(telemetry_net_info ws2_32 rpcrt4 ncrypt winhttp secur32 crypt32)
else()
    target_link_libraries(telemetry_net_info m)
endif()
