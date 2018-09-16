#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

function(add_memory_directory whatIsBuilding)
    add_analytic_directory(${whatIsBuilding} "network_info")
endfunction()

add_memory_directory(telemetry_net_info "network_info")

if (${use_prov_client})
    #add_analytic_directory(prov_net_info "network_info")
endif()
