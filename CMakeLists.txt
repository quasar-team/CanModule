cmake_minimum_required(VERSION 3.11)
include(FetchContent)

project(CanModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(src/include
                    ${PROJECT_BINARY_DIR}/src/include/)

include(cmake/version.cmake)

set(SOURCES
    src/main/CanFrame.cpp
    src/main/CanDevice.cpp
    src/main/CanStats.cpp
    src/main/CanVendorLoopback.cpp
    src/main/CanDeviceConfiguration.cpp
)

include(cmake/env.cmake)


set(VENDOR_SOURCES
    src/main/CanVendorAnagate.cpp
)

if (UNIX)
    list(APPEND VENDOR_SOURCES src/main/CanVendorSocketCan.cpp)
endif()

include(cmake/logit.cmake)
include(cmake/anagate.cmake)

add_library(CanModuleMain ${SOURCES} ${VENDOR_SOURCES})
target_include_directories(CanModuleMain PRIVATE ${logit_SOURCE_DIR}/include ${anagate_SOURCE_DIR}/include)
if (UNIX)
    target_link_libraries(CanModuleMain PRIVATE
                            ${anagate_SOURCE_DIR}/lib/libAnaGateExtStaticRelease.so
                            ${anagate_SOURCE_DIR}/lib/libAnaGateStaticRelease.so
                            ${anagate_SOURCE_DIR}/lib/libCANDLLStaticRelease64.so
                            socketcan)
    file(GLOB ANAGATE_LIBRARIES_SO ${anagate_SOURCE_DIR}/lib/*.so)
    file(COPY ${ANAGATE_LIBRARIES_SO} DESTINATION ${CMAKE_BINARY_DIR})
else()
    target_link_libraries(CanModuleMain PRIVATE
                            ${anagate_SOURCE_DIR}/lib/AnaGateCanDll64.lib)
    file(COPY "${anagate_SOURCE_DIR}/lib/AnaGateCan64.dll" DESTINATION "${CMAKE_BINARY_DIR}/Release")
endif()


# Treat warnings as errors
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(CanModuleMain PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # target_compile_options(CanModuleMain PRIVATE /W4 /WX)
    # I need to disable that feature due to a macro redefinition
    # in the header of Anagate software
endif()

include(cmake/pybind11.cmake)
include(cmake/gtest.cmake)
