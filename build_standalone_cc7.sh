#!/bin/bash
rm -rf ./CMakeCache.txt
# build standalone libs
export CANMODULE_AS_STATIC_AS_POSSIBLE=true
#export CANMODULE_AS_STATIC_AS_POSSIBLE=false
CMAKEBIN=/usr/local/bin/cmake


# copy/swap  CMakeLists.txt
#mv ./CMakeLists.txt ./CMakeLists.txt.org
#mv ./CMakeLists.standalone.txt ./CMakeLists.txt

# out of source build, no toolchain
${CMAKEBIN} -S . -B ./build .
cd ./build && ${CMAKEBIN} --build . -- -j5
#
# swap back CMakeLists.txt
#mv ./CMakeLists.txt ./CMakeLists.standalone.txt
#mv ./CMakeLists.txt.org ./CMakeLists.txt


