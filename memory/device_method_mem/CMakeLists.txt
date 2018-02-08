#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(device_method_mem_c_files
    sdk_mem_analytics.c
    ../mem_analytics.c
    ../../../certs/certs.c
)

set(device_method_mem_h_files
    sdk_mem_analytics.h
    ../mem_analytics.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)

include_directories(. ..)
include_directories(${IOTHUB_CLIENT_INC_FOLDER})
include_directories(${DEV_AUTH_MODULES_CLIENT_INC_FOLDER})
include_directories(${SHARED_UTIL_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/adapters)

add_executable(device_method_mem ${device_method_mem_c_files} ${device_method_mem_h_files})

if(${use_openssl})
    add_definitions(-DUSE_OPENSSL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_OPENSSL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_OPENSSL")
    if (WIN32)
        target_link_libraries(device_method_mem $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
        file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    endif()
endif()

target_link_libraries(device_method_mem iothub_client)

if (${use_prov_client})
    target_link_libraries(device_method_mem prov_device_ll_client)
endif()

if (${use_http})
    if (${use_prov_client})
        target_link_libraries(device_method_mem prov_http_transport)
    endif()
    add_definitions(-DUSE_HTTP)
endif()
if (${use_mqtt})
    target_link_libraries(device_method_mem iothub_client_mqtt_transport iothub_client_mqtt_ws_transport)
    if (${use_prov_client})
        target_link_libraries(device_method_mem prov_mqtt_transport prov_mqtt_ws_transport)
    endif()
    add_definitions(-DUSE_MQTT)
endif()
if (${use_amqp})
    target_link_libraries(device_method_mem iothub_client_amqp_transport iothub_client_amqp_ws_transport)
    if (${use_prov_client})
        target_link_libraries(device_method_mem prov_amqp_ws_transport prov_amqp_transport)
    endif()
    add_definitions(-DUSE_AMQP)
endif()

if (${use_prov_client})
    link_security_client(device_method_mem)
endif()
linkSharedUtil(device_method_mem)
linkUAMQP(device_method_mem)

