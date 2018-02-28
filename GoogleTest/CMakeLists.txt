find_package(Threads REQUIRED)

# activate cmake external project module
include(ExternalProject)


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
elseif(MSVC)#The following lines will only execute in the compiler is Visual C++
	set_target_properties(libgtest PROPERTIES
		"IMPORTED_LOCATION" "${binary_dir}/${CMAKE_BUILD_TYPE}/gtest.lib"
		"IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREADS_LIBS_INIT}"
		#"INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/include"
	)
endif()
