cmake_minimum_required(VERSION 3.11)
include(FetchContent)

project(CanModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(
    src/include
    ${PROJECT_BINARY_DIR}/src/include/
)

include(cmake/version.cmake)

set(SOURCES
    src/main/CanFrame.cpp
    src/main/CanDevice.cpp
    src/main/CanVendorLoopback.cpp
    src/main/CanDeviceConfiguration.cpp
    src/main/CanDiagnostics.cpp
    src/main/CanDerivedStats.cpp
)

include(cmake/env.cmake)

set(VENDOR_SOURCES
    src/main/CanVendorAnagate.cpp
)

if (UNIX)
    include(cmake/libsocketcan.cmake)
    include_directories(${LIBSOCKETCAN_INCLUDE_DIR})
    list(APPEND VENDOR_SOURCES
        src/main/CanVendorSocketCan.cpp
        src/main/CanVendorSocketCanSystec.cpp
    )
endif()

if (NOT DEFINED CAN_MODULE_MAIN_ONLY)
    include(cmake/logit.cmake)
    set(LOGIT_INCLUDE ${logit_SOURCE_DIR}/include)
else()
    set(LOGIT_INCLUDE ${CMAKE_SOURCE_DIR}/LogIt/include)
endif()

include(cmake/anagate.cmake)


add_library(CanModuleMain ${SOURCES} ${VENDOR_SOURCES})

target_include_directories(CanModuleMain PUBLIC
    ${LOGIT_INCLUDE}
    ${anagate_SOURCE_DIR}/
)

if (UNIX)
    target_link_libraries(CanModuleMain PUBLIC
        ${anagate_SOURCE_DIR}/Linux64/CentOS_9/libCANDLLStaticRelease64.a
        ${anagate_SOURCE_DIR}/Linux64/CentOS_9/libAnaGateStaticRelease.a
        ${anagate_SOURCE_DIR}/Linux64/CentOS_9/libAnaGateExtStaticRelease.a
        libsocketcan
    )
else()
    target_link_libraries(CanModuleMain PUBLIC
        ${anagate_SOURCE_DIR}/Win64/AnaGateCanDll64.lib
    )
    file(COPY "${anagate_SOURCE_DIR}/Win64/AnaGateCan64.dll"
         DESTINATION "${CMAKE_BINARY_DIR}/Release")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(CanModuleMain PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # Disabled because of AnaGate macro redefinition issues
endif()

if (NOT DEFINED CAN_MODULE_MAIN_ONLY)
    include(cmake/pybind11.cmake)
    include(cmake/gtest.cmake)
endif()
