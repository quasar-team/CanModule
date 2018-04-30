cmake_minimum_required(VERSION 2.8)
project(stcan)

if(NOT WIN32)
	MESSAGE( FATAL_ERROR "No systec build available for this platform [${CMAKE_SYSTEM_NAME}]. For linux use the socketCAN implementation with the SYSTEC driver" )
endif()

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

set(STCAN_SOURCES
	STCanScan.cpp
	STCanScan.cpp
	${CMAKE_SOURCE_DIR}/CanInterface/src/CanStatistics.cpp
	${CMAKE_SOURCE_DIR}/CanInterface/include/CanStatistics.h	
)

include_directories ( "C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Include" )

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( stcan SHARED ${STCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( stcan SHARED ${STCAN_SOURCES} )
	target_link_libraries( stcan ${LOGIT_LIB} )
endif()

link_directories( "C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Lib/" )
target_link_libraries ( stcan ${BOOST_LIBS} USBCAN32 )

set_property(TARGET stcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET stcan PROPERTY LINKER_LANGUAGE CXX)