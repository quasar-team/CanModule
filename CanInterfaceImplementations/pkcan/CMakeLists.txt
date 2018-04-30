cmake_minimum_required(VERSION 2.8)
project(pkcan)

if(NOT WIN32)
	MESSAGE( FATAL_ERROR "No peakCAN build currently available for this platform [${CMAKE_SYSTEM_NAME}]. For linux use the socketCAN implementation with the PEAK driver" )
endif()

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

set(PEAKCAN_SOURCES
	pkcan.cpp
	pkcan.h
	${CMAKE_SOURCE_DIR}/CanInterface/src/CanStatistics.cpp
	${CMAKE_SOURCE_DIR}/CanInterface/include/CanStatistics.h	
	${CMAKE_SOURCE_DIR}/CanInterface/src/CanModuleUtils.cpp
	${CMAKE_SOURCE_DIR}/CanInterface/include/CanModuleUtils.h	
)

include_directories ( "C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/Include" )

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( pkcan SHARED ${PEAKCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( pkcan SHARED ${PEAKCAN_SOURCES} )
	target_link_libraries( pkcan ${LOGIT_LIB} )
endif()

target_link_libraries ( pkcan ${BOOST_LIBS} "C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/x64/VC_LIB/PCANBasic.lib" )

set_property(TARGET pkcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET pkcan PROPERTY LINKER_LANGUAGE CXX)