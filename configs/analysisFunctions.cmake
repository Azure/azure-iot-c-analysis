#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

function(add_analytic_directory whatIsBuilding folder_name)
    add_subdirectory(${whatIsBuilding})
    set_target_properties(${whatIsBuilding} PROPERTIES FOLDER ${folder_name})
endfunction(add_analytic_directory)

function(compileTargetAsC99 theTarget)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set_target_properties(${theTarget} PROPERTIES COMPILE_FLAGS "--std=c99")
    endif()
  else()
    set_target_properties(${theTarget} PROPERTIES C_STANDARD 99)
    set_target_properties(${theTarget} PROPERTIES CXX_STANDARD 11)
  endif()
endfunction()
