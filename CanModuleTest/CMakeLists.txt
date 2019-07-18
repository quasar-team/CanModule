cmake_minimum_required(VERSION 2.8)
project(CanModuleTest)
find_package(Threads REQUIRED)

function ( clone_googletest GOOGLETEST_VERSION)
  message(STATUS "cloning googletest from github. *NOTE* cloning [${GOOGLETEST_VERSION}]")
  execute_process(COMMAND git clone -b ${GOOGLETEST_VERSION} https://github.com/google/googletest.git WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  message(STATUS "googletest cloned to [${PROJECT_SOURCE_DIR}/googletest]")    
endfunction()

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

file(GLOB CAN_MODULE_TEST_SRCS src/*.cpp)
include_directories( 
	include 
	../CanInterfaceImplementations/unitTestMockUpImplementation/
)

clone_googletest("master")
set( BUILD_GTEST ON CACHE BOOL "We do want to build gtest")
set( BUILD_GMOCK OFF CACHE BOOL "we do not want to build gmock")
set( gtest_force_shared_crt ON CACHE BOOL "use shared CRT" FORCE)
add_subdirectory(googletest)

add_executable( CanModuleTest ${CAN_MODULE_TEST_SRCS} )

message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using GTESTLIB [${GTESTLIB}]")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using BOOST_LIBS ${BOOST_LIBS}")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")

SET ( CANMODULE_LIB_NAME "CanModule-static" )

target_link_libraries (CanModuleTest
	${BOOST_LIBS}
	${CANMODULE_LIB_NAME}
	${CMAKE_THREAD_LIBS_INIT}
	${LOGIT_LIB}
	gtest
	-ldl
)

add_dependencies (CanModuleTest ${CANMODULE_LIB_NAME} gtest)

