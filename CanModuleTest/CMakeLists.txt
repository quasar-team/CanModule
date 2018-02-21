cmake_minimum_required(VERSION 2.8)
find_package(Threads REQUIRED)

add_library(custgtest STATIC IMPORTED)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
find_library(
  GTEST_LIB NAMES gtest libgtest 
  PATHS ${PROJECT_SOURCE_DIR}/GoogleTest/gtest/src/gtest-build/ NO_DEFAULT_PATH
)
message(STATUS "GTEST_LIB [${GTEST_LIB}]")

SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".lib")
find_library(
  CANMODULE_LIB NAMES CanModule libCanModule 
  PATHS ${PROJECT_SOURCE_DIR} NO_DEFAULT_PATH
)
message(STATUS "CANMODULE_LIB [${CANMODULE_LIB}]")

file(GLOB CAN_MODULE_TEST_SRCS src/*.cpp)
include_directories(
	include
	../GoogleTest/gtest/src/gtest/include
)

add_executable( CanModuleTest ${CAN_MODULE_TEST_SRCS} )

target_link_libraries (CanModuleTest
	${GTEST_LIB}
	${CANMODULE_LIB}
	${CMAKE_THREAD_LIBS_INIT}
	${BOOST_LIBS}
	${LOGIT_LIB}
)

message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")

IF (WIN32)
ELSE()
	target_link_libraries (CanModuleTest -ldl)
ENDIF()

add_dependencies (CanModuleTest gtest libgtest CanModule)