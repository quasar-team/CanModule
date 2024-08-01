cmake_minimum_required(VERSION 3.26)

project(CanModule LANGUAGES CXX)

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

add_library(CanModuleMain ${SOURCES})

# Treat warnings as errors
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(CanModuleMain PRIVATE -Wall -Wextra -Werror)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(CanModuleMain PRIVATE /W4 /WX)
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

add_executable(CanModuleTest ${TEST_SOURCES})

target_link_libraries(
  CanModuleTest CanModuleMain gtest_main
  #pybind11::module ${Python3_LIBRARIES}
)

include(GoogleTest)
gtest_discover_tests(CanModuleTest)


# PYBIND11 SECTION

set(PYTHON_SOURCES
    src/python/CanFrame.cpp
)

# Check if pybind11 is installed using pip
execute_process(
    COMMAND ${CMAKE_COMMAND} -E env python -m pip show pybind11
    RESULT_VARIABLE pybind11_installed
    OUTPUT_VARIABLE pybind11_output
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(pybind11_installed EQUAL 0)
  message(STATUS "pybind11 found, genering python module")

  # Extract the location of pybind11
  string(REGEX MATCH "Location: ([^\n]+)" _ ${pybind11_output})
  string(REGEX REPLACE "Location: " "" pybind11_location ${CMAKE_MATCH_1})

  # Set the pybind11_DIR to the location of pybind11
  set(pybind11_DIR ${pybind11_location}/pybind11/share/cmake/pybind11)
  include_directories(${pybind11_location}/pybind11/include)
  include_directories(/usr/include/python3.9/)

  find_package(pybind11 REQUIRED)

  pybind11_add_module(canmodule ${PYTHON_SOURCES} ${SOURCES})

  target_link_libraries(canmodule PRIVATE pybind11::module ${PYTHON_LIBRARIES})

  # Add a preprocessor definition to enable Python bindings in the code
  target_compile_definitions(canmodule PRIVATE PYTHON_BINDINGS_ENABLED)

  # Link Python libraries
  find_package(PythonLibs REQUIRED)
  find_package(Python3 REQUIRED COMPONENTS Development)
endif()
