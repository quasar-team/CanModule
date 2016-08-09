cmake_minimum_required(VERSION 2.8)
find_package(Threads REQUIRED)

IF (WIN32)
	if(NOT TARGET custgtest)
		add_library(custgtest STATIC IMPORTED)
		set_property(TARGET custgtest PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/CanModule/GoogleTest/gtest/src/gtest-build/Release/gtest.lib)
	endif()
ELSE()
	if(NOT TARGET custgtest)
		add_library(custgtest STATIC IMPORTED)
		set_property(TARGET custgtest PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/CanModule/GoogleTest/gtest/src/gtest-build/libgtest.a)
	endif()
ENDIF()

file(GLOB CAN_MODULE_TEST_SRCS src/*.cpp)
include_directories(
	include
	../GoogleTest/gtest/src/gtest/include
	)

add_executable(CanModuleTest 
	$<TARGET_OBJECTS:CanModule>
	$<TARGET_OBJECTS:LogIt>
	${CAN_MODULE_TEST_SRCS}
)

target_link_libraries (CanModuleTest
	custgtest
	${CMAKE_THREAD_LIBS_INIT}
	${BOOST_LIBS}
)


IF (WIN32)

ELSE()
	add_definitions(-DBOOST_ALL_DYN_LINK)
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fPIC -rdynamic")
	target_link_libraries (CanModuleTest -ldl -lpthread ${BOOST_LIBS})
ENDIF()

add_dependencies (CanModuleTest libgtest CanModule LogIt)