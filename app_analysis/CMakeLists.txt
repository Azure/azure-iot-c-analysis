#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(app_analysis_h_files
    inc/binary_handler.h
    inc/process_handler.h
)

set(app_analysis_c_files
    src/binary_handler.c
    src/main.c
)

if(WIN32)
    set(app_analysis_c_files
        ${app_analysis_c_files}
        src/process_handler_win.c
    )
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    set(app_analysis_c_files
        ${app_analysis_c_files}
        src/process_handler_linux.c
    )
endif(WIN32)

include_directories(${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/inc)
include_directories(${SHARED_UTIL_INC_FOLDER})

#set(BINARY_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

add_executable(app_analysis ${app_analysis_c_files} ${app_analysis_h_files})
target_link_libraries(app_analysis aziotsharedutil)

compileTargetAsC99(app_analysis)