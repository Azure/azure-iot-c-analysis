#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

function(add_memory_directory whatIsBuilding)
    add_analytic_directory(${whatIsBuilding} "analysis-memory")
    #add_subdirectory(${whatIsBuilding})

    #set_target_properties(${whatIsBuilding} PROPERTIES FOLDER "memory")
endfunction()

#add_memory_directory(device_method_mem)
add_memory_directory(c2d_memory)
add_memory_directory(telemetry_memory)
