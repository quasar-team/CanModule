cmake_minimum_required(VERSION 2.8)
project(pkcan)

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

if(0)
if(WIN32)
	SET(PEAKCAN_INC_HINT "C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/Include")
	SET(PEAKCAN_LIB_HINT "C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/x64/VC_LIB/PCANBasic.lib")
else()
	SET(PEAKCAN_INC_HINT "/some/path/to/include/directory/")
	SET(PEAKCAN_LIB_HINT "/some/path/to/lib/binary")
endif()
endif()

# check if toolchain injection was correct for peak
if( NOT DEFINED PEAK_PATH_LIBS ) 
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable PEAK_LIB_FILE has not been defined, take default." )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable PEAK_INC_DIR has not been defined, take default." )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable PEAK_PATH_LIBS has not been defined, take default." )
	if(WIN32)  
			SET( PEAK_INC_DIR "C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/Include")
			SET( PEAK_PATH_LIBS "C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/x64/VC_LIB")
			SET( PEAK_LIB_FILE "PCANBasic.lib")
	else()
		message(ERROR "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: this directory is only needed for PEAK/windows, it must be skipped for Linux]")
	endif() 
ENDIF()

message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: PEAK_INC_DIR= ${PEAK_INC_DIR}]")
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: PEAK_PATH_LIBS= ${PEAK_PATH_LIBS}]")
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: PEAK_LIB_FILE= ${PEAK_LIB_FILE}]")
 
set(PEAKCAN_SOURCES
	pkcan.cpp
	pkcan.h
	../../CanInterface/src/CanStatistics.cpp
	../../CanInterface/include/CanStatistics.h	
	../../CanInterface/src/CanModuleUtils.cpp
	../../CanInterface/include/CanModuleUtils.h	
)

# checkCANImplResourceExists(PEAKCAN_INC_DIR ${PEAKCAN_INC_HINT})
include_directories ( ${PEAKCAN_INC_DIR} )

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( pkcan SHARED ${PEAKCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( pkcan SHARED ${PEAKCAN_SOURCES} )
	target_link_libraries( pkcan ${LOGIT_LIB} )
endif()

#checkCANImplResourceExists(PEAKCAN_LIB_FILE ${PEAKCAN_LIB_HINT})
target_link_libraries ( pkcan ${BOOST_LIBS} ${PEAKCAN_LIB_FILE} )

set_property(TARGET pkcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET pkcan PROPERTY LINKER_LANGUAGE CXX)
