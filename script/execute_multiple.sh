#!/bin/bash
#set -o pipefail
#

set -e

gcc --version
openssl version
uname -r

script_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "${script_dir}/.." && pwd)
cmake_folder=$repo_root"/cmake/analysis_multiple"

declare -a arr=(
    "lts_10_2018"
    "2018-09-11"
    "master"
)

for item in "${arr[@]}"
do
    rm -r -f $cmake_folder
    # Remove the SDK
    rm -r -f "$repo_root/deps/c-sdk"
    mkdir -p $cmake_folder
    pushd $cmake_folder

    echo "executing cmake/make with options <<$item>>"
    cmake $repo_root -DCMAKE_BUILD_TYPE=Release -Dsdk_branch="$item" -Duse_http=OFF

    echo "building SDK"
    # Run make, but don't show the output
    make -j >/dev/null

    # Run the analysis applications
    echo "Retrieving binary info"
    ./binary_info/binary_info -c $cmake_folder
done
popd
