find_package(Threads REQUIRED)

# activate cmake external project module
include(ExternalProject)

function ( clone_googletest GOOGLETEST_VERSION)
  message(STATUS "cloning googletest from github. *NOTE* cloning [${GOOGLETEST_VERSION}]")
  execute_process(COMMAND git clone -b ${GOOGLETEST_VERSION} https://github.com/google/googletest.git)
  message(STATUS "googletest cloned")    
endfunction()

function ( build_googletest )
  message(STATUS "generating platform specific build for googletest library for [${CMAKE_GENERATOR}]")
  execute_process( COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}")
  message(STATUS "executing platform specific build for googletest library")
  execute_process( COMMAND "${CMAKE_COMMAND}")
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
    ${CMAKE_CURRENT_LIST_DIR}/GoogleTest/gtest/src/gtest-build/googlemock/ 
    ${CMAKE_CURRENT_LIST_DIR}/GoogleTest/gtest/src/gtest-build/googlemock/Debug 
    ${CMAKE_CURRENT_LIST_DIR}/GoogleTest/gtest/src/gtest-build/googlemock/Release
    NO_DEFAULT_PATH
)

## libgtest - as a target depenedency for other modules in this build
#add_library(libgtest IMPORTED STATIC GLOBAL)
#add_dependencies(libgtest gtest)
#
#ExternalProject_Get_Property(gtest source_dir binary_dir)
#if(CMAKE_COMPILER_IS_GNUCXX)
#	set(GTEST_LIB_FILE_PATH "${binary_dir}/googlemock/gtest/libgtest.a")
#elseif(MSVC)#The following lines will only execute in the compiler is Visual C++
#  set(GTEST_LIB_FILE_NAME "gtest.lib")
#  if(CMAKE_BUILD_TYPE STREQUAL "DEBUG")
#    set(GTEST_LIB_FILE_NAME "gtestd.lib")
#  endif()
#  set(GTEST_LIB_FILE_PATH "${binary_dir}/googlemock/gtest/${CMAKE_BUILD_TYPE}/${GTEST_LIB_FILE_NAME}")
#endif()
#
#message(STATUS "searching for gtest library file [${GTEST_LIB_FILE_PATH}]")
#set_target_properties(libgtest PROPERTIES
#	"IMPORTED_LOCATION" ${GTEST_LIB_FILE_PATH}
#	"IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREADS_LIBS_INIT}"
#	#"INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/include"
#)
#
