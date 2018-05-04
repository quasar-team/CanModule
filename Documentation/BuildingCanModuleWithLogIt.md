# Building CanModule and LogIt

CanModule can be built as a stand-alone (shared) library for windows and linux. CanModule depends on LogIt
to provide the familiar LogIt interface ```(LOG << "message ["<<some_var<<"]")``` and back-end implementations
that deliver logged messages to the appropriate endpoint (console, rotating file buffer, etc).

Two options exist to satisfy CanModule's LogIt dependency:
1) The LogIt code is simply cloned into CanModule source tree during CanModule cmake generation. The LogIt
   code is then simply built into the CanModule library.
2) LogIt is built externally to CanModule as a (shared) library. In this case the CanModule cmake generation
   step must be informed as to where the LogIt includes and binary library reside.

The options above are controlled by the LOGIT_BUILD_OPTION build option.

Of these 2 options, the first is simple, offering a quick and easy way to obtain a CanModule shared library
however the 2nd offers the most flexibility: Take the example of an executable (e.g. an OPC-UA server) using 
CanModule. There exist dependencies on LogIt on 3 levels in this server:
1. The server depends directly on LogIt; the server code contains direct calls to the LogIt API. 
2. The CanModule shared library depends directly on LogIt; CanModule source code directly calls LogIt.
3. CAN implementation shared libraries (socketCAN, anagate...) similarly directly depends on LogIt.

Such an executable should use a single LogIt shared library to satisfy these dependencies. So, CanModule
should be built with option 2 (LOGIT_BUILD_OPTION=LOGIT_AS_EXT_SHARED). This is what is described below.

Note the examples below use the cmake out-of-source build whereby the end-user creates an empty build
directory and directs cmake to generate makefiles and output build artefacts in that directory. This is
a fairly common cmake approach, here we use this approach to cleanly seperate debug and release builds 
(only release shown here).


After the build steps outlined below you should have
1. LogIt shared library + include directory
2. CanModule shared library + include directory + implementation shared libraries


## Windows (using the git-bash mingw64 terminal)
```
# where is boost ?
#   this windows build uses 'boost_custom_win_VS2017.cmake' (see below) to locate boost
#      'boost_custom_win_VS2017.cmake' NEEDS these environment variables.
#
#   Note: boost builds on windows result in many permutations regarding exactly what the resulting
#   boost library names are (it depends on the options/tools used for the boost build). Furthermore the 
#   permutation list has been observed to change between boost versions. Rather than LogIt/CanModule 
#   trying to provide a single boost locator file to cover every permutation (considered a losing
#   battle) the approach advised is that end-users copy 'boost_custom_win_VS2017.cmake' to make their
#   own boost locator cmake extension, making the neccesary edits to suit the specifics of their
#   particular boost installation.
#
#   If anyone feels they can write the one-size-fits-all windows boost locator file then great. Please
#   contribute it to the repo as a new file.
#
#   CAUTION! BACKEND_BOOSTLOG needs boost > v1.54.0
export BOOST_HOME=/c/workspace/boost_mapped_namespace_builder/work/MAPPED_NAMESPACE_INSTALL/
export BOOST_PATH_LIBS=/c/workspace/boost_mapped_namespace_builder/work/MAPPED_NAMESPACE_INSTALL/lib/

# clone LogIt, prepare for out-of-source build
git clone https://github.com/quasar-team/LogIt.git
mkdir LogIt/build-release
cd LogIt/build-release/

# generate VS2017 solution
cmake -DCMAKE_BUILD_TYPE=Release \
      -DLOGIT_BUILD_SHARED_LIB=ON \
      -DLOGIT_BACKEND_STDOUTLOG=ON -DLOGIT_BACKEND_BOOSTLOG=ON \
      -DCMAKE_TOOLCHAIN_FILE=boost_custom_win_VS2017.cmake \
      -G "Visual Studio 15 2017 Win64" ..

# clone CanModule, prepare for out-of-source build
git clone https://github.com/quasar-team/CanModule.git
mkdir CanModule/build-release
cd CanModule/build-release/

# decide which implementations to build by editing CanInterfaceImplementations/CMakeLists.txt
# below you only need to specify the XXX_INC_DIR & XXX_LIB_FILE for the active implementations.

# generate VS2017 solution
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSTANDALONE_BUILD=ON \       
      -DSYSTEC_INC_DIR="C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Include" \
      -DSYSTEC_LIB_FILE="C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Lib/USBCAN64.lib" -DPEAKCAN_INC_DIR="C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/Include" \
      -DPEAKCAN_LIB_FILE="C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/x64/VC_LIB/PCANBasic.lib" \
      -DANAGATE_INC_DIR="C:/3rdPartySoftware/AnaGateCAN/win64/vc8/include" \
      -DANAGATE_LIB_FILE="C:/3rdPartySoftware/AnaGateCAN/win64/vc8/Release/AnaGateCanDll64.lib" \
      -DLOGIT_BUILD_OPTION=LOGIT_AS_EXT_SHARED \
      -DLOGIT_EXT_LIB_DIR=/c/workspace/OPC-UA/Wiener/LogIt/build-release/Release/ \
      -DLOGIT_INCLUDE_DIR=/c/workspace/OPC-UA/Wiener/LogIt/include/ \
      -DCMAKE_TOOLCHAIN_FILE=boost_custom_win_VS2017.cmake \
      -G "Visual Studio 15 2017 Win64" ..
```

## Linux (CC7)
```
# where is boost ?
#   if you build with a custom boost version then cmake should use file 'boost_custom_cc7.cmake' (see below) 
#      'boost_custom_cc7.cmake NEEDS these environment variables.
#   if you build with whatever boost is installed on your system then cmake should use file boost_standard_install_cc7.cmake (see below) 
#      boost_standard_install_cc7.cmake does not need these environment variables. You can omit this step.
#
#   CAUTION! BACKEND_BOOSTLOG needs boost > v1.54.0
export BOOST_PATH_HEADERS=/local/bfarnham/workspace/boost_mapped_namespace_builder/work/MAPPED_NAMESPACE_INSTALL/64bit/include/
export BOOST_PATH_LIBS=/local/bfarnham/workspace/boost_mapped_namespace_builder/work/MAPPED_NAMESPACE_INSTALL/64bit/lib/

# clone LogIt, prepare for out-of-source build
git clone https://github.com/quasar-team/LogIt.git
mkdir LogIt/build-release
cd LogIt/build-release/

# generate makefile
cmake -DCMAKE_BUILD_TYPE=Release \
      -DLOGIT_BUILD_SHARED_LIB=ON \
      -DLOGIT_BACKEND_STDOUTLOG=ON -DLOGIT_BACKEND_BOOSTLOG=ON \
      -DCMAKE_TOOLCHAIN_FILE=boost_custom_cc7.cmake ..

# clone CanModule, prepare for out-of-source build
git clone https://github.com/quasar-team/CanModule.git
mkdir CanModule/build-release
cd CanModule/build-release/

# decide which implementations to build by editing CanInterfaceImplementations/CMakeLists.txt
# below you only need to specify the XXX_INC_DIR & XXX_LIB_FILE for the active implementations.

cmake -DCMAKE_BUILD_TYPE=Release \
      -DSTANDALONE_BUILD=ON \
      -DANAGATE_INC_DIR="/3rdPartySoftware/Anagate/anagate-api-2.13/linux64/gcc4_6/include/" \
      -DANAGATE_LIB_FILE="/3rdPartySoftware/Anagate/anagate-api-2.13/linux64/gcc4_6/libAPIRelease64.so" \
      -DLOGIT_BUILD_OPTION=LOGIT_AS_EXT_SHARED \
      -DLOGIT_EXT_LIB_DIR=/local/bfarnham/workspace/quasar_stuff/LogIt/build-release/ \
      -DLOGIT_INCLUDE_DIR=/local/bfarnham/workspace/quasar_stuff/LogIt/include/ \
      -DCMAKE_TOOLCHAIN_FILE=boost_custom_cc7.cmake ..
```
