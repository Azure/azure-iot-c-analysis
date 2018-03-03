#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#compileAsC99()

set(source_c_files
    provisioning_mem.c
    ../mem_analytics.c
    ../../mem_reporter.c
    ../../certs/certs.c
)

set(source_h_files
    provisioning_mem.h
    ../../mem_reporter.h
    ../../certs/certs.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

add_definitions(-DUSE_PROVISIONING)
add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC -DGB_DEBUG_NETWORK -DGB_MEASURE_NETWORK_FOR_THIS)

include_directories(. .. ../..)
include_directories(${SDK_INCLUDE_DIRS})

add_executable(provisioning_mem ${source_c_files} ${source_h_files})

if(${use_openssl})
    add_definitions(-DUSE_OPENSSL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_OPENSSL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_OPENSSL")
    if (WIN32)
        target_link_libraries(provisioning_mem $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
        file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    endif()
elseif(${use_wolfssl})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_WOLFSSL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_WOLFSSL")
endif()

link_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})

target_link_libraries(provisioning_mem 
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
    target_link_libraries(provisioning_mem ws2_32 rpcrt4 ncrypt secur32 crypt32)
else()
    target_link_libraries(provisioning_mem m)
endif()
