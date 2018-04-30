cmake_minimum_required(VERSION 2.8)
project(pkcan)

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

if(WIN32)
	SET(PEAKCAN_INC_HINT "C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/Include")
	SET(PEAKCAN_LIB_HINT "C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/x64/VC_LIB/PCANBasic.lib")
else()
	SET(PEAKCAN_INC_HINT "/some/path/to/include/directory/")
	SET(PEAKCAN_LIB_HINT "/some/path/to/lib/binary")
endif()

set(PEAKCAN_SOURCES
	pkcan.cpp
	pkcan.h
	${CMAKE_SOURCE_DIR}/CanInterface/src/CanStatistics.cpp
	${CMAKE_SOURCE_DIR}/CanInterface/include/CanStatistics.h	
	${CMAKE_SOURCE_DIR}/CanInterface/src/CanModuleUtils.cpp
	${CMAKE_SOURCE_DIR}/CanInterface/include/CanModuleUtils.h	
)

checkCANImplResourceExists(PEAKCAN_INC_DIR ${PEAKCAN_INC_HINT})
include_directories ( ${PEAKCAN_INC_DIR} )

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( pkcan SHARED ${PEAKCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( pkcan SHARED ${PEAKCAN_SOURCES} )
	target_link_libraries( pkcan ${LOGIT_LIB} )
endif()

checkCANImplResourceExists(PEAKCAN_LIB_FILE ${PEAKCAN_LIB_HINT})
target_link_libraries ( pkcan ${BOOST_LIBS} ${PEAKCAN_LIB_FILE} )

set_property(TARGET pkcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET pkcan PROPERTY LINKER_LANGUAGE CXX)