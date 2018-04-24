cmake_minimum_required(VERSION 2.8)

include_directories (
	${PROJECT_SOURCE_DIR}/CanModule/CanInterface/include
	${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/sockcan
)

link_directories( /usr/local/lib )

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)

set(SOCKCAN_SOURCES
	SockCanScan.cpp
	SockCanScan.h
	../../CanInterface/src/CanStatistics.cpp
	../../CanInterface/src/CanModuleUtils.cpp
)

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( SocketCanImplementation SHARED ${SOCKCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( SocketCanImplementation SHARED ${SOCKCAN_SOURCES} )
	target_link_libraries( SocketCanImplementation ${LOGIT_LIB} )
endif()

target_link_libraries ( SocketCanImplementation ${BOOST_LIBS} libsocketcan.a )

add_definitions(-std=gnu++0x)
set_property(TARGET SocketCanImplementation PROPERTY POSITION_INDEPENDENT_CODE TRUE)

