#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
project(azure_iot_c_analysis)

include("configs/analysisFunctions.cmake")

set(TARGET_GIT_BRANCH "master" CACHE STRING "Branch of the SDK repo to target`")

option(not_using_blob_upload "use http upload to blob" ON)

if(WIN32)
    option(use_openssl "set use_openssl to ON if openssl is to be used, set to OFF to not use openssl" OFF)
    option(use_wolfssl "set use_wolfssl to ON if wolfssl is to be used, set to OFF to not use wolfssl" OFF)
else()
    option(use_openssl "set use_openssl to ON if openssl is to be used, set to OFF to not use openssl" ON)
    option(use_wolfssl "set use_wolfssl to ON if wolfssl is to be used, set to OFF to not use wolfssl" OFF)
endif()

include(ExternalProject)

message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

# load the c-sdk
#SET(CMAKE_BUILD_TYPE Release)
#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

# Check out Git submodules.
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/c-sdk/.gitmodules")
    message("running git clone of the c-sdk into ${CMAKE_CURRENT_SOURCE_DIR}/deps/c-sdk")
    execute_process (COMMAND git clone https://github.com/Azure/azure-iot-sdk-c.git --recursive ${CMAKE_CURRENT_SOURCE_DIR}/deps/c-sdk)
    #execute_process (COMMAND git submodule update --init --recursive WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(memory_trace ON)
set(skip_samples ON)

add_subdirectory(deps/c-sdk)

set(SDK_INCLUDE_DIRS 
    ${CMAKE_SOURCE_DIR}/deps/c-sdk/iothub_client/inc
    ${CMAKE_SOURCE_DIR}/deps/c-sdk/iothub_service_client/inc
    ${CMAKE_SOURCE_DIR}/deps/c-sdk/c-utility/inc
    ${CMAKE_SOURCE_DIR}/deps/c-sdk/provisioning_client/inc
)

add_subdirectory(memory)
add_analytic_directory(binary_info "binary_info")
