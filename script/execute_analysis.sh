#!/bin/bash
#set -o pipefail
#

set -e

gcc --version
openssl version
uname -r

script_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "${script_dir}/.." && pwd)
cmake_folder=$repo_root"/cmake/analysis_linux"

conn_string="${IOTHUB_CONNECTION_STRING}"

rm -r -f $cmake_folder
mkdir -p $cmake_folder
pushd $cmake_folder

echo "Running cmake with -DCMAKE_BUILD_TYPE=Release"
cmake $repo_root -DCMAKE_BUILD_TYPE=Release  >/dev/null # -Duse_prov_client:BOOL=ON

echo "Building SDK"
make -j >/dev/null

# Run strip from the binaries
#./binary_info/lower_layer/

# Run the analysis applications
echo "Retrieving binary info"
./binary_info/binary_info -c $cmake_folder
echo "retrieving telemetry memory info"
./memory/telemetry_memory/telemetry_memory -c $conn_string
echo "retrieving telemetry network info"
./network/telemetry_net_info/telemetry_net_info -c $conn_string

#./memory/prov_net_info/prov_net_info -c $DPS_C_CONNECTION_STRING -s $DPS_C_SCOPE_ID_VALUE
#./memory/provisioning_mem/provisioning_mem -c $DPS_C_CONNECTION_STRING -s $DPS_C_SCOPE_ID_VALUE
