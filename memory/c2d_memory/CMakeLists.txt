#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(c2d_memory_c_files
    c2d_mem_analytics.c
    ../mem_analytics.c
    ../../mem_reporter.c
    ../../certs/certs.c
)

set(c2d_memory_h_files
    c2d_mem_analytics.h
    ../../mem_reporter.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

add_definitions(-DUSE_C2D)
add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)

include_directories(. .. ../..)
include_directories(${SDK_INCLUDE_DIRS})

add_executable(c2d_memory ${c2d_memory_c_files} ${c2d_memory_h_files})

if(${use_openssl})
    add_definitions(-DUSE_OPENSSL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_OPENSSL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_OPENSSL")
    if (WIN32)
        target_link_libraries(c2d_memory $ENV{OpenSSLDir}/lib/ssleay32.lib $ENV{OpenSSLDir}/lib/libeay32.lib)
        file(COPY $ENV{OpenSSLDir}/bin/libeay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
        file(COPY $ENV{OpenSSLDir}/bin/ssleay32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    endif()
endif()

link_directories(${IOTHUB_CLIENT_BIN_DIR})

#target_link_libraries(c2d_memory 
#    ${IOTHUB_CLIENT_BIN_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}iothub_client${CMAKE_STATIC_LIBRARY_SUFFIX}
#    ${IOTHUB_CLIENT_BIN_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}iothub_client_mqtt_transport${CMAKE_STATIC_LIBRARY_SUFFIX}
#    ${IOTHUB_CLIENT_BIN_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}iothub_client_mqtt_ws_transport${CMAKE_STATIC_LIBRARY_SUFFIX}
#    ${IOTHUB_CLIENT_BIN_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}iothub_client_amqp_transport${CMAKE_STATIC_LIBRARY_SUFFIX}
#    ${IOTHUB_CLIENT_BIN_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}iothub_client_amqp_ws_transport${CMAKE_STATIC_LIBRARY_SUFFIX}
#    ${IOTHUB_CLIENT_BIN_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}iothub_client_http_transport${CMAKE_STATIC_LIBRARY_SUFFIX}
#    ${UAMQP_BIN_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}uamqp${CMAKE_STATIC_LIBRARY_SUFFIX}
#    ${UMQTT_BIN_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}umqtt${CMAKE_STATIC_LIBRARY_SUFFIX}
#    ${C_UTIL_BIN_DIR}${CMAKE_STATIC_LIBRARY_PREFIX}aziotsharedutil${CMAKE_STATIC_LIBRARY_SUFFIX}
#)
target_link_libraries(c2d_memory 
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
    target_link_libraries(c2d_memory ws2_32 rpcrt4 ncrypt winhttp secur32 crypt32)
else()
    target_link_libraries(c2d_memory m)
endif()

