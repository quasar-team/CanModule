# © Copyright CERN, Geneva, Switzerland, 2016.  All rights not expressly granted are reserved.
#
#   Created on: Wed Aug 30 10:15:57 CEST 2017
# 
#  		Author: Michael Ludwig <michael.ludwig@cern.ch>
#       Contributors and prior art: Benjamin Farnham, Piotr Nikiel, Viacheslav Filimonov
# 
#  This file is part of the CAN Common Tools project and is the property of CERN, Geneva, Switzerland,
#  and is not free software, since it builds on top of vendor
#  specific communication interfaces and architectures, which are generally non-free and
#  are subject to licensing and/or registration. Please refer to the relevant
#  collaboration agreements between CERN ICS and the vendors for further details.
# 
#  The non-vendor specific parts of the software can be made available on request
#  under the GNU Lesser General Public Licence,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public Licence for more details <http://www.gnu.org/licenses/>.
#  
# Authors: Viatcheslav Filimonov, Piotr Nikiel, Ben Farnham, Michael Ludwig ("the quasar team from Atlas and BE-ICS")
#
cmake_minimum_required(VERSION 2.8)
project(stcan)

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

# check if toolchain injection was correct for systec
# for windows we should use the default systec installer and location
# for linux we need libsocketcan
if( NOT DEFINED SYSTEC_LIB_PATH ) 
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable SYSTEC_LIB_FILE has not been defined, take default." )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable SYSTEC_INC_DIR has not been defined, take default." )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable SYSTEC_LIB_PATH has not been defined, take default." )
	if(WIN32)
		# using the default installer/location provided by systec for windows
		SET(SYSTEC_INC_DIR "C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Include")
		SET(SYSTEC_LIB_PATH "C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Lib")
		if( 64BIT )  
			SET(SYSTEC_LIB_FILE "USBCAN64.lib")
		else()
			SET(SYSTEC_LIB_FILE "USBCAN32.lib")
		endif()
		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: windows SYSTEC_LIB_PATH= ${SYSTEC_LIB_PATH}]")
	else()
	    # use socketcan for linux, get sources 
	    # from i.e. ssh://git@gitlab.cern.ch:7999/mludwig/CAN_libsocketcan.git and build it
		SET(SYSTEC_HEADERS "/opt/CAN_libsocketcan/include")
		SET(SYSTEC_PATH_LIBS "/opt/CAN_libsocketcan/src/.libs")
		SET(SYSTEC_LIB_FILE "libsocketcan.so")
		message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: default path might not work, please get socketcan and correct it." )
		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: linux SYSTEC_LIB_PATH= ${SYSTEC_LIB_PATH}]")
	endif()
endif()

link_directories( ${SYSTEC_PATH_LIBS} )
include_directories ( ${SYSTEC_HEADERS} )

set(STCAN_SOURCES
	STCanScan.cpp
	STCanScan.cpp
	../../CanInterface/src/CanStatistics.cpp
	../../CanInterface/include/CanStatistics.h	
	../../CanInterface/src/CanModuleUtils.cpp
	../../CanInterface/include/CanModuleUtils.h	
)

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( stcan SHARED ${STCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( stcan SHARED ${STCAN_SOURCES} )
	target_link_libraries( stcan ${LOGIT_LIB} )
endif()

target_link_libraries ( stcan ${BOOST_LIBS} ${SYSTEC_LIB_FILE})
set_property(TARGET stcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET stcan PROPERTY LINKER_LANGUAGE CXX)

message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] systec [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] systec [BOOST_HEADERS:${BOOST_HEADERS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] systec [BOOST_LIBS:${BOOST_LIBS}]" )

link_directories( ${BOOST_PATH_LIBS} )

