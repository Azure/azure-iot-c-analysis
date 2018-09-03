#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(iothub_c_files
    ../../ul_sdk_client.c
    ${PROJECT_SOURCE_DIR}/certs/certs.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(${SDK_CERT_INCLUDE_DIRS})
include_directories(${BINARY_SOURCE_DIR} ${REPORTER_DIR})

add_executable(mqtt_ws_transport_ul ${iothub_c_files})
target_link_libraries(mqtt_ws_transport_ul iothub_client)
target_link_libraries(mqtt_ws_transport_ul iothub_client_mqtt_ws_transport)
linkMqttLibrary(mqtt_ws_transport_ul)
add_definitions(-DUSE_MQTT)
add_definitions(-DUSE_WEB_SOCKETS)
