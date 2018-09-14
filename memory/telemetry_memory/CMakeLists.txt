#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#compileAsC99()

set(telemetry_memory_c_files
    sdk_mem_analytics.c
    ../mem_analytics.c
    ../../mem_reporter.c
    ../../certs/certs.c
)

set(telemetry_memory_h_files
    sdk_mem_analytics.h
    ../../mem_reporter.h
    ../../certs/certs.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

add_definitions(-DUSE_TELEMETRY)
add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
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

include_directories(${CMAKE_CURRENT_LIST_DIR} ${REPORTER_DIR} ${REPORTER_DIR}/deps/parson)
include_directories(${SDK_INCLUDE_DIRS})

add_executable(telemetry_memory ${telemetry_memory_c_files} ${telemetry_memory_h_files})

if(${use_openssl})
    add_definitions(-DUSE_OPENSSL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_OPENSSL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_OPENSSL")
    if (WIN32)
        target_link_libraries(telemetry_memory $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
        file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    endif()
elseif(${use_wolfssl})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_WOLFSSL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_WOLFSSL")
endif()

link_directories(${IOTHUB_CLIENT_BIN_DIR})

target_link_libraries(telemetry_memory 
    iothub_client
    aziotsharedutil
)

if (${use_mqtt})
    target_link_libraries(telemetry_memory 
        iothub_client_mqtt_transport
        iothub_client_mqtt_ws_transport
        umqtt
    )
endif()
if (${use_amqp})
    target_link_libraries(telemetry_memory 
        iothub_client_amqp_transport
        iothub_client_amqp_ws_transport
        uamqp
    )
endif()
if (${use_http})
    target_link_libraries(telemetry_memory 
        iothub_service_client
        iothub_client_http_transport
    )
endif()

if(WIN32)
    target_link_libraries(telemetry_memory ws2_32 rpcrt4 ncrypt winhttp secur32 crypt32)
else()
    target_link_libraries(telemetry_memory m)
endif()
