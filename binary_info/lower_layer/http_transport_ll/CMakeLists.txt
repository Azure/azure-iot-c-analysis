#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(iothub_c_files
    ../../ll_sdk_client.c
    ${SDK_CERT_INCLUDE_DIRS}/certs.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(${SDK_CERT_INCLUDE_DIRS})
include_directories(${BINARY_SOURCE_DIR} ${REPORTER_DIR})

add_executable(http_transport_ll ${iothub_c_files})
target_link_libraries(http_transport_ll iothub_client)
target_link_libraries(http_transport_ll iothub_client_http_transport)
add_definitions(-DUSE_HTTP)
