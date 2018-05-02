cmake_minimum_required(VERSION 2.8)
project(stcan)

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

if(WIN32)
	SET(SYSTEC_INC_HINT "C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Include")
	SET(SYSTEC_LIB_HINT "C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Lib/USBCAN64.lib")
else()
	SET(SYSTEC_INC_HINT "/some/path/to/include/directory/")
	SET(SYSTEC_LIB_HINT "/some/path/to/lib/binary")
endif()

set(STCAN_SOURCES
	STCanScan.cpp
	STCanScan.cpp
	../../CanInterface/src/CanStatistics.cpp
	../../CanInterface/include/CanStatistics.h	
	../../CanInterface/src/CanModuleUtils.cpp
	../../CanInterface/include/CanModuleUtils.h	
)

checkCANImplResourceExists(SYSTEC_INC_DIR ${SYSTEC_INC_HINT})
include_directories ( ${SYSTEC_INC_DIR} )

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( stcan SHARED ${STCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( stcan SHARED ${STCAN_SOURCES} )
	target_link_libraries( stcan ${LOGIT_LIB} )
endif()

checkCANImplResourceExists(SYSTEC_LIB_FILE ${SYSTEC_LIB_HINT})
target_link_libraries ( stcan ${BOOST_LIBS} ${SYSTEC_LIB_FILE})

set_property(TARGET stcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET stcan PROPERTY LINKER_LANGUAGE CXX)
