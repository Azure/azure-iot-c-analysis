#!/bin/bash
#set -o pipefail
#

set -e

gcc --version
openssl version

script_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "${script_dir}/.." && pwd)
cmake_folder=$repo_root"/cmake/analysis_linux"

declare -a arr=(
    "-Dsdk_branch=lts_10_2018"
    "-Dsdk_branch=2018-09-11"
    "-Dsdk_branch=2018-07-11"
    "-Dsdk_branch=master"
)

for item in "${arr[@]}"
do
    rm -r -f $build_folder
    mkdir -p $build_folder
    pushd $cmake_folder

    echo "executing cmake/make with options <<$item>>"
    cmake $repo_root -DCMAKE_BUILD_TYPE=Release -Dsdk_branch="$item"

    # Run make, but don't show the output
    make -j --silent

    # Run the analysis applications
    echo "Retrieving binary info"
    ./binary_info/binary_info -c $cmake_folder
done
popd
