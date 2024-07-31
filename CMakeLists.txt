cmake_minimum_required(VERSION 3.26)

project(CanCommon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(src/include)

# Source files
set(SOURCES
    src/main/CanFrame.cpp
    src/main/CanDevice.cpp
    src/main/CanStats.cpp
    src/main/CanExtra.cpp
    src/main/CanVendorAnagate.cpp
    src/main/CanVendorDummy.cpp
    src/main/CanVendorSocketCan.cpp
)

add_library(CanCommon ${SOURCES})

# Treat warnings as errors
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(CanCommon PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(CanCommon PRIVATE /W4 /WX)
endif()

# GOOGLE TESTS SECTION

enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add test files
set(TEST_SOURCES
    test/cpp/CanDevice_test.cpp
    test/cpp/CanFrame_test.cpp
    test/cpp/CanExtra_test.cpp
    test/cpp/CanStats_test.cpp
)

add_executable(CanCommon_test ${TEST_SOURCES})

target_link_libraries(
  CanCommon_test CanCommon gtest_main
)

include(GoogleTest)
gtest_discover_tests(CanCommon_test)
