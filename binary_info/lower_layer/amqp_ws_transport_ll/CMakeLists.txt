#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(iothub_c_files
    ../../ll_sdk_client.c
    ${PROJECT_SOURCE_DIR}/certs/certs.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(${PROJECT_SOURCE_DIR}/certs)

include_directories(.. ../..)

add_executable(amqp_ws_transport_ll ${iothub_c_files})
target_link_libraries(amqp_ws_transport_ll iothub_client)
target_link_libraries(amqp_ws_transport_ll iothub_client_amqp_ws_transport)
linkUAMQP(amqp_ws_transport_ll)
add_definitions(-DUSE_AMQP)
add_definitions(-DUSE_WEB_SOCKETS)
