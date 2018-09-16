#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

function(add_memory_directory whatIsBuilding)
    add_analytic_directory(${whatIsBuilding} "memory_info")
    #add_subdirectory(${whatIsBuilding})
endfunction()

if (${use_http})
    add_analytic_directory(telemetry_memory "heap_analysis")
    #add_memory_directory(device_method_mem)
    #add_analytic_directory(c2d_memory "heap_analysis")
    #add_analytic_directory(telemetry_net_info "network_info")
endif()

if (${use_prov_client})
    #add_analytic_directory(prov_net_info "network_info")
    #add_analytic_directory(provisioning_mem "prov_mem")
endif()
