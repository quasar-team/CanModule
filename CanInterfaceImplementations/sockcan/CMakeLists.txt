cmake_minimum_required(VERSION 2.8)
project(sockcan)

link_directories( /usr/local/lib )

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

set(SOCKCAN_SOURCES
	SockCanScan.cpp
	SockCanScan.h
	${CMAKE_SOURCE_DIR}/CanInterface/src/CanStatistics.cpp
	${CMAKE_SOURCE_DIR}/CanInterface/include/CanStatistics.h	
)

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( sockcan SHARED ${SOCKCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( sockcan SHARED ${SOCKCAN_SOURCES} )
	target_link_libraries( sockcan ${LOGIT_LIB} )
endif()

target_link_libraries ( sockcan ${BOOST_LIBS} libsocketcan.a )

set_property(TARGET sockcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET sockcan PROPERTY LINKER_LANGUAGE CXX)
