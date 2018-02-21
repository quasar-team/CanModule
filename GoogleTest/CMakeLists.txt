find_package(Threads REQUIRED)

# activate cmake external project module
include(ExternalProject)

ExternalProject_Add(
	gtest
	URL https://github.com/google/googletest/archive/release-1.7.0.zip
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
	INSTALL_COMMAND "" #disabled
	CMAKE_ARGS -Dgtest_force_shared_crt=ON
)

# libgtest - as a target depenedency for other modules in this build
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

ExternalProject_Get_Property(gtest source_dir binary_dir)
if(CMAKE_COMPILER_IS_GNUCXX)
	set_target_properties(libgtest PROPERTIES
		"IMPORTED_LOCATION" "${binary_dir}/libgtest.a"
		"IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREADS_LIBS_INIT}"
		#"INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/include"
	)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")#The following lines will only execute in the compiler is Visual C++
	set_target_properties(libgtest PROPERTIES
		"IMPORTED_LOCATION" "${binary_dir}/${CMAKE_BUILD_TYPE}/gtest.lib"
		"IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREADS_LIBS_INIT}"
		#"INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/include"
	)
endif()
