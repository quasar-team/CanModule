cmake_minimum_required(VERSION 2.8)
project(GoogleTest)
find_package(Threads REQUIRED)

function ( clone_googletest GOOGLETEST_VERSION)
  message(STATUS "cloning googletest from github. *NOTE* cloning [${GOOGLETEST_VERSION}]")
  execute_process(COMMAND git clone -b ${GOOGLETEST_VERSION} https://github.com/google/googletest.git WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  message(STATUS "googletest cloned")    
endfunction()

function ( build_googletest )
  message(STATUS "generating platform specific build for googletest library for [${CMAKE_GENERATOR}]")
  execute_process( COMMAND "${CMAKE_COMMAND}"  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DBUILD_GTEST=ON -DBUILD_GMOCK=OFF -G "${CMAKE_GENERATOR}" WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/googletest)
  message(STATUS "executing platform specific build for googletest library")
  execute_process( COMMAND "${CMAKE_COMMAND}" --build . WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/googletest)
endfunction()

set(GOOGLETEST_VERSION "release-1.8.0")
if(MSVC)
  message(STATUS "for vs2017 use gtest master - the gtest v1.8.0 build fails out of the box. The master branch has a fix")
  set(GOOGLETEST_VERSION "master")
endif()

clone_googletest(${GOOGLETEST_VERSION})
build_googletest()

SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
find_library(
  GTESTLIB NAMES gtest gtestd 
  PATHS 
    ${PROJECT_BINARY_DIR}/googletest/googletest 
    ${PROJECT_BINARY_DIR}/googletest/googletest/Debug 
    ${PROJECT_BINARY_DIR}/googletest/googletest/Release
    NO_DEFAULT_PATH
)

if(NOT GTESTLIB)
  message(FATAL_ERROR "Failed to find gtest library - build will stop PROJECT_BINARY_DIR [${PROJECT_BINARY_DIR}]")
else()
  message(STATUS "Found gtest library GTESTLIB [${GTESTLIB}]")
endif()