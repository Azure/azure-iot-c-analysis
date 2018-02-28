#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC99()

set(iothub_c_files
    ../../ll_dps_client.c
    ${PROJECT_SOURCE_DIR}/certs/certs.c
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(${PROJECT_SOURCE_DIR}/certs ${PROJECT_SOURCE_DIR}/provisioning_client/inc)

include_directories(.. ../.. )

add_executable(prov_mqtt_transport_ll ${iothub_c_files})
target_link_libraries(prov_mqtt_transport_ll prov_device_ll_client)
target_link_libraries(prov_mqtt_transport_ll prov_mqtt_transport)

target_link_libraries(prov_mqtt_transport_ll prov_auth_client hsm_security_client)

linkMqttLibrary(prov_mqtt_transport_ll)
add_definitions(-DUSE_MQTT)
