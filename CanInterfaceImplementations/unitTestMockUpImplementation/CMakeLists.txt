cmake_minimum_required(VERSION 2.8)
project(MockUpCanImplementationcan)

message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: STANDALONE_BUILD [${STANDALONE_BUILD}]")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

set(MOCK_SOURCES
	MockCanAccess.cpp
	MockCanAccess.h
	../../CanInterface/src/CanStatistics.cpp
	../../CanInterface/include/CanStatistics.h	
)


link_directories( ${BOOST_PATH_LIBS} )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_HEADERS:${BOOST_HEADERS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_LIBS:${BOOST_LIBS}]" )


if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( MockUpCanImplementationcan SHARED ${MOCK_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( MockUpCanImplementationcan SHARED ${MOCK_SOURCES} )
	target_link_libraries( MockUpCanImplementationcan ${LOGIT_LIB} )
endif()
target_link_libraries ( MockUpCanImplementationcan	${BOOST_LIBS} )

set_property(TARGET MockUpCanImplementationcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET MockUpCanImplementationcan PROPERTY LINKER_LANGUAGE CXX)
