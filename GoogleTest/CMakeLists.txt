find_package(Threads REQUIRED)

# activate cmake external project module
include(ExternalProject)

function ( build_googletest )
  message(STATUS "generating platform specific build for googletest library for [${CMAKE_GENERATOR}]")
  execute_process( COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}")
  message(STATUS "executing platform specific build for googletest library")
  execute_process( COMMAND "${CMAKE_COMMAND}")
endfunction()

set(GTEST_ZIP_URL "https://github.com/google/googletest/archive/release-1.8.0.zip")
if(MSVC)
  message(STATUS "for vs2017 use gtest master - the gtest v1.8.0 build fails out of the box. The master branch has a fix")
  set(GTEST_ZIP_URL "https://github.com/google/googletest/archive/master.zip")
endif()

message(STATUS "downloading gtest archive from URL [${GTEST_ZIP_URL}]")
ExternalProject_Add(
	gtest
	URL ${GTEST_ZIP_URL}
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
	INSTALL_COMMAND "" #disabled
	CMAKE_ARGS -Dgtest_force_shared_crt=ON
)
build_googletest()

# libgtest - as a target depenedency for other modules in this build
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

ExternalProject_Get_Property(gtest source_dir binary_dir)
if(CMAKE_COMPILER_IS_GNUCXX)
	set(GTEST_LIB_FILE_PATH "${binary_dir}/googlemock/gtest/libgtest.a")
elseif(MSVC)#The following lines will only execute in the compiler is Visual C++
  set(GTEST_LIB_FILE_NAME "gtest.lib")
  if(CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    set(GTEST_LIB_FILE_NAME "gtestd.lib")
  endif()
  set(GTEST_LIB_FILE_PATH "${binary_dir}/googlemock/gtest/${CMAKE_BUILD_TYPE}/${GTEST_LIB_FILE_NAME}")
endif()

message(STATUS "searching for gtest library file [${GTEST_LIB_FILE_PATH}]")
set_target_properties(libgtest PROPERTIES
	"IMPORTED_LOCATION" ${GTEST_LIB_FILE_PATH}
	"IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREADS_LIBS_INIT}"
	#"INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/include"
)

