cmake_minimum_required(VERSION 3.11)
include(FetchContent)

project(CanModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(src/include)

set(SOURCES
    src/main/CanFrame.cpp
    src/main/CanDevice.cpp
    src/main/CanStats.cpp
    src/main/CanExtra.cpp
    src/main/CanVendorLoopback.cpp
)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND git log -1 --format=%cd
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file (
    "${PROJECT_SOURCE_DIR}/src/include/CanVersion.h.in"
    "${PROJECT_BINARY_DIR}/src/include/CanVersion.h"
    @ONLY
)

set(VENDOR_SOURCES
    src/main/CanVendorAnagate.cpp
)

if (UNIX)
    list(APPEND VENDOR_SOURCES src/main/CanVendorSocketCan.cpp)
endif()

FetchContent_Declare(
  LogIt
  GIT_REPOSITORY https://github.com/quasar-team/LogIt.git
  GIT_TAG        master
  DOWNLOAD_EXTRACT_TIMESTAMP True
)
FetchContent_MakeAvailable(LogIt)

add_library(CanModuleMain ${SOURCES} ${VENDOR_SOURCES})
target_include_directories(CanModuleMain PRIVATE ${logit_SOURCE_DIR}/include)

# Treat warnings as errors
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(CanModuleMain PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(CanModuleMain PRIVATE /W4 /WX)
endif()

include(CMakeLists.pybind11.cmake)
include(CMakeLists.gtest.cmake)
