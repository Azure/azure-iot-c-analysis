#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(binary_info_c_files
    binary_info.c
    ${REPORTER_DIR}/mem_reporter.c
    ${REPORTER_DIR}/deps/parson/parson.c
)

set(binary_info_h_files
    ${REPORTER_DIR}/mem_reporter.h
    ${REPORTER_DIR}/deps/parson/parson.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

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
set(BINARY_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

add_executable(binary_info ${binary_info_c_files} ${binary_info_h_files})
target_link_libraries(binary_info aziotsharedutil iothub_client)

if (${use_prov_client})
    target_link_libraries(binary_info prov_device_client)
endif()

add_subdirectory(lower_layer)
add_subdirectory(upper_layer)
