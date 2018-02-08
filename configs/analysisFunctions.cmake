#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

function(add_analytic_directory whatIsBuilding folder_name)
    add_subdirectory(${whatIsBuilding})
    set_target_properties(${whatIsBuilding} PROPERTIES FOLDER ${folder_name})
endfunction(add_analytic_directory)
