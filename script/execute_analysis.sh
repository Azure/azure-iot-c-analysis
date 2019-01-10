#!/bin/bash
#set -o pipefail
#
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

# Print version
cat /etc/*release | grep VERSION*
gcc --version
openssl version
uname -r

cmake_cmd=""
run_binary_strip=0
script_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "${script_dir}/.." && pwd)
cmake_folder=$repo_root"/cmake/analysis_linux"

conn_string="${IOTHUB_CONNECTION_STRING}"
rpt_conn_string="${REPORT_CONNECTION_STRING}"

run_binary_info()
{
    #rm -r -f $cmake_folder
    #mkdir -p $cmake_folder
    pushd $cmake_folder  >/dev/null

    #cmake $repo_root $cmake_cmd >/dev/null
    #make -j >/dev/null

    if [ $run_binary_strip -ge 1 ]; then
        # Run strip from the binaries./
        strip ./binary_info/lower_layer/mqtt_transport_ll/mqtt_transport_ll
        strip ./binary_info/lower_layer/mqtt_ws_transport_ll/mqtt_ws_transport_ll
        strip ./binary_info/lower_layer/amqp_transport_ll/amqp_transport_ll
        strip ./binary_info/lower_layer/amqp_ws_transport_ll/amqp_ws_transport_ll
    fi

    ./binary_info/binary_info -c $cmake_folder -s $rpt_conn_string
    echo ""
    popd  >/dev/null
}

echo "Running cmake with -DCMAKE_BUILD_TYPE=Release"
cmake_cmd="-DCMAKE_BUILD_TYPE=Release"
run_binary_info

echo "Running cmake with -DCMAKE_BUILD_TYPE=Release no_logging"
cmake_cmd="-DCMAKE_BUILD_TYPE=Release -Dno_logging=ON"
run_binary_info

echo "Running cmake with -DCMAKE_BUILD_TYPE=Release no_logging and strip"
cmake_cmd="-DCMAKE_BUILD_TYPE=Release -Dno_logging=ON"
run_binary_strip=1
run_binary_info

# Run the analysis applications
pushd $cmake_folder

echo "retrieving telemetry memory info"
./memory/telemetry_memory/telemetry_memory -c $conn_string
echo "retrieving telemetry network info"
./network/telemetry_net_info/telemetry_net_info -c $conn_string

#./memory/prov_net_info/prov_net_info -c $DPS_C_CONNECTION_STRING -s $DPS_C_SCOPE_ID_VALUE
#./memory/provisioning_mem/provisioning_mem -c $DPS_C_CONNECTION_STRING -s $DPS_C_SCOPE_ID_VALUE
