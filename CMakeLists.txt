#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
project(azure_iot_c_analysis)

include("configs/analysisFunctions.cmake")

set(TARGET_GIT_BRANCH "master" CACHE STRING "Branch of the SDK repo to target`")

set(use_prov_client OFF)
set(sdk_branch "master" CACHE STRING "The branch of the sdk")

option(dont_use_uploadtoblob "set dont_use_uploadtoblob to ON if the functionality of upload to blob is to be excluded, OFF otherwise. It requires HTTP" ON)
option(refresh_sdk "get the latest sdk" OFF)
option(memory_trace "" ON)
option(skip_samples "set skip_samples to ON to skip building samples (default is OFF)[if possible, they are always build]" ON)

include(ExternalProject)

# Check out Git submodules.
#if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/c-sdk/.gitmodules")
#    message("running git clone of the c-sdk into ${CMAKE_CURRENT_SOURCE_DIR}/deps/c-sdk")
#    execute_process (COMMAND git clone https://github.com/Azure/azure-iot-sdk-c.git --recursive -q --no-tags -b ${sdk_branch} ${CMAKE_CURRENT_SOURCE_DIR}/deps/c-sdk)
#else()
#    if (${refresh_sdk})
#        execute_process (COMMAND git pull origin ${sdk_branch} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deps/c-sdk")
#        # Turn off refresh so it doesn't refresh everytime
#        set(refresh_sdk OFF CACHE BOOL "" FORCE)
#    endif()
#endif()

#if(${no_logging})
#    add_definitions(-DNO_LOGGING)
#endif()
#if (${dont_use_uploadtoblob})
#    add_definitions(-DDONT_USE_UPLOADTOBLOB)
#endif()

# load the c-sdk
#SET(CMAKE_BUILD_TYPE Release)
#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

#add_definitions(-DGB_DEBUG_NETWORK -DGB_MEASURE_NETWORK_FOR_THIS)

#add_subdirectory(deps/c-sdk)

#set(SDK_INCLUDE_DIRS 
#    ${CMAKE_SOURCE_DIR}/deps/c-sdk/certs
#    ${CMAKE_SOURCE_DIR}/deps/c-sdk/iothub_client/inc
#    ${CMAKE_SOURCE_DIR}/deps/c-sdk/iothub_service_client/inc
#    ${CMAKE_SOURCE_DIR}/deps/c-sdk/c-utility/inc
#)
#if (${use_prov_client})
#    set(SDK_INCLUDE_DIRS ${SDK_INCLUDE_DIRS}
#        ${CMAKE_SOURCE_DIR}/deps/c-sdk/provisioning_client/inc)
#endif()

#set(SDK_CERT_INCLUDE_DIRS 
#    ${CMAKE_SOURCE_DIR}/deps/c-sdk/certs
#)
#set(REPORTER_DIR ${CMAKE_CURRENT_LIST_DIR})

#add_analytic_directory(binary_info "binary_info")
#add_subdirectory(network)
#add_subdirectory(memory)

add_subdirectory(deps/c-utility)
include_directories(${SHARED_UTIL_INC_FOLDER})

add_analytic_directory(app_analysis "app_analysis")

add_subdirectory(test_files)
