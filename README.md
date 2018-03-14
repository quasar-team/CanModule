# CanModule
CanModule is a cross-platform library for controlling any kind of CAN device. All CAN devices are represented by a simple abstract interface (class CanModule::CCanAccess) - user code uses this interface (*only* this interface) to send messages, receive messages, etc. i.e. to interact with the CAN device as per the needs of the application. Of course, abstract interfaces require concrete implementations - these implementations are a kind of functional mapping; driving underlying CAN hardware through some lower level API in a way that satisifies the behaviour 'described' in the CCanAccess interface. CanModule comes out-of-the-box with implementations for certain CAN devices (see table below). Implementations for other CAN devices can be added - please submit a pull request with your implementation for review.

How does it work? CanModule

## Currently Available Implementations
| Linux  | Windows |
| ------------- | ------------- |
| Systec (via [socketCAN](https://www.kernel.org/doc/Documentation/networking/can.txt))  |Systec (via the Peak windows API)  |
| Peak (via [socketCAN](https://www.kernel.org/doc/Documentation/networking/can.txt))  | Peak (via the Peak windows API)  |
| Anagate (via the Anagate linux API)  | Anagate (via the Anagate windows API)  |
| *Mock* implementation (simulates CAN hardware, for testing)  | *Mock* implementation (simulates CAN hardware, for testing) |
| **Note**: **any** CAN device with a socketCAN driver is supported via **CanModule's socketCAN implementation** | Windows doesn't have an equivalent to socketCAN, sadly |

## Building CanModule
Before you try it, you're going to have to build it. There are a few pre-requisite requirements...
1. CanModule relies on [cmake](cmake.org) for handling cross-platform builds: install cmake.
2. CanModule uses C++ [boost](boost.org), it simplifies our cross-platform coding overhead. Install boost.
   
   ⋅⋅⋅A little note on boost here: you can use the system installation of boost or some custom boost build if that's what you need. More on this later.
3. CanModule uses LogIt. [LogIt](github.com/quasar-team/LogIt) is the quasar team's standard logging library.

   ⋅⋅⋅A little note on LogIt too: you can build LogIt directy into your CanModule library (i.e. as source code), or you can have CanModule link to LogIt as an external shared library.
   
### Building on Linux

_example: build CanModule as a shared library using the system boost installation. Note in this case [LogIt](github.com/quasar-team/LogIt) is built directly in to the CanModule library - i.e. LogIt is not linked as an external library)._
```
cmake -DSTANDALONE_BUILD=ON -DCMAKE_TOOLCHAIN_FILE=boost_standard_install_cc7.cmake
```
Note: this also builds the unit tests (in CanModuleTest, built with googletest) to verify that the essential mechanisms in the CanModule work as expected. More on these tests later, for now we just verify that they run and pass
```
# from directory CanModule 

# 1 amend the LD_LIBRARY_PATH so that the CanModuleTest can load the MockUpCanImplementation.so
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/CanModule/CanInterfaceImplementations/output

#2 run the CanModuleTest
CanModuleTest/CanModuleTest
```

_example: build CanModule as a shared library using a custom boost version (e.g. built locally from source). Use LogIt as an external shared library (i.e. LogIt was built in some independent build). Note the 3 **LOGIT_<etc>** build parameters defining the location of the directories containing the LogIt shared library and the LogIt header files#_
```
# (comment) - set environment variables to point to the custom boost headers/libs directories (required by the toolchain file)
export BOOST_PATH_HEADERS=/local/bfarnham/workspace/boost_mapped_namespace_builder/work/MAPPED_NAMESPACE_INSTALL/64bit/include/
export BOOST_PATH_LIBS=/local/bfarnham/workspace/boost_mapped_namespace_builder/work/MAPPED_NAMESPACE_INSTALL/64bit/lib/

cmake -DSTANDALONE_BUILD=ON -DCMAKE_TOOLCHAIN_FILE=boost_custom_cc7.cmake -DLOGIT_BUILD_OPTION=LOGIT_AS_EXT_SHARED -DLOGIT_EXT_LIB_DIR=/local/bfarnham/workspace/quasar_stuff/LogIt/ -DLOGIT_EXT_INC_DIR=/local/bfarnham/workspace/quasar_stuff/LogIt/include

make clean && make
```

### Building on Windows
