#!/bin/bash
#set -o pipefail
#

set -e

script_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "${script_dir}/.." && pwd)
cmake_folder=$repo_root"/cmake/analysis_linux"

rm -r -f $cmake_folder
mkdir -p $cmake_folder
pushd $cmake_folder
cmake $repo_root -DCMAKE_BUILD_TYPE=Release -Dskip_samples:BOOL=ON -Duse_prov_client:BOOL=ON -Ddont_use_uploadtoblob:BOOL=OFF

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

# Make sure there is enough virtual memory on the device to handle more than one job  
MINVSPACE="1500000"

# Acquire total memory and total swap space setting them to zero in the event the command fails
MEMAR=( $(sed -n -e 's/^MemTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' -e 's/^SwapTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' /proc/meminfo) )
[ -z "${MEMAR[0]##*[!0-9]*}" ] && MEMAR[0]=0
[ -z "${MEMAR[1]##*[!0-9]*}" ] && MEMAR[1]=0

let VSPACE=${MEMAR[0]}+${MEMAR[1]}

make --jobs=$CORES

# Run the analysis applications
./binary_info/binary_info -c $cmake_folder
./memory/telemetry_memory/telemetry_memory -c $IOTHUB_CONNECTION_STRING -d jebrandoDevice -k "P3YJYsVQwytQP2JrLca9lSCvyYfPDVjiCZR+5apSQ7c="
./memory/network_info/network_info -c $IOTHUB_CONNECTION_STRING -d jebrandoDevice -k "P3YJYsVQwytQP2JrLca9lSCvyYfPDVjiCZR+5apSQ7c="
./memory/provisioning_mem/provisioning_mem -c "global.azure-devices-provisioning.net" -s $PROV_SCOPE_ID
