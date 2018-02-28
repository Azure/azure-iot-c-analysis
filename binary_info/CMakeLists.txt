#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(binary_info_c_files
    binary_info.c
    ../mem_reporter.c
)

set(binary_info_h_files
    ../mem_reporter.h
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)

if (${use_prov_client})
    add_definitions(-DUSE_PROVISIONING_CLIENT)
endif()

include_directories(. ..)
include_directories(${SDK_INCLUDE_DIRS})

add_executable(binary_info ${binary_info_c_files} ${binary_info_h_files})
target_link_libraries(binary_info aziotsharedutil iothub_client)

target_link_libraries(binary_info prov_device_client)

add_subdirectory(lower_layer)
add_subdirectory(upper_layer)
