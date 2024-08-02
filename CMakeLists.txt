cmake_minimum_required(VERSION 3.26)

project(CanModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src/include)

set(SOURCES
    src/main/CanFrame.cpp
    src/main/CanDevice.cpp
    src/main/CanStats.cpp
    src/main/CanExtra.cpp
    src/main/CanVendorAnagate.cpp
    src/main/CanVendorDummy.cpp
    src/main/CanVendorSocketCan.cpp
)

add_library(CanModuleMain ${SOURCES})

# Treat warnings as errors
# if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#     target_compile_options(CanModuleMain PRIVATE -Wall -Wextra -Werror)
# elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#     target_compile_options(CanModuleMain PRIVATE /W4 /WX)
# endif()

include(CMakeLists.gtest.txt)
include(CMakeLists.pybind11.txt)
