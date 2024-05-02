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
cmake_minimum_required(VERSION 3.0)
project( stcan LANGUAGES C CXX )

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib )

# check if toolchain injection was correct for systec
# for windows we should use the default systec installer and location
# for linux we need libsocketcan
if( (NOT DEFINED SYSTEC_LIB_PATH) OR (NOT DEFINED SYSTEC_HEADERS) OR (NOT DEFINED SYSTEC_LIB_FILE)  )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable SYSTEC_HEADERS has not been defined." )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable SYSTEC_LIB_PATH has not been defined." )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable SYSTEC_LIB_FILE has not been defined." )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: assuming default locations and libs for systec and windows" )
	if(WIN32)
		SET(SYSTEC_HEADERS "C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Include")
		SET(SYSTEC_LIB_PATH "C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Lib")
		#SET(SYSTEC_LIB_FILE "USBCAN64.lib")
		# for 32 bit 
		SET(SYSTEC_LIB_FILE "USBCAN32.dll")
		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: windows SYSTEC_LIB_PATH= ${SYSTEC_LIB_PATH}]")
 		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: windows SYSTEC_HEADERS=  ${SYSTEC_HEADERS}]")
 		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: windows SYSTEC_LIB_FILE= ${SYSTEC_LIB_FILE}]")
	else()
	    # since we use socketcan for linux for systec, we are in the wrong directory
		message( FATAL_ERROR "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: linux uses socketcan (directory sockcan) for systec, you are building the wrong directory." )
	endif()
else()
  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: systec SYSTEC_LIB_PATH= ${SYSTEC_LIB_PATH}" )
  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: systec SYSTEC_HEADERS=  ${SYSTEC_HEADERS}" )
  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: systec SYSTEC_LIB_FILE = ${SYSTEC_LIB_FILE}" )
endif() 

include_directories ( ${SYSTEC_HEADERS}  )
link_directories( ${SYSTEC_LIB_PATH} )

set(STCAN_SOURCES
	STCanScan.cpp
	../../CanInterface/src/CanStatistics.cpp	
	../../CanInterface/src/CanModuleUtils.cpp	
	../../CanInterface/src/CCanAccess.cpp	
)

SET ( SYSTEC_LIB_NAME "stcan" )
if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( ${SYSTEC_LIB_NAME} SHARED ${STCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( ${SYSTEC_LIB_NAME} SHARED ${STCAN_SOURCES} )
	target_link_libraries( ${SYSTEC_LIB_NAME} ${LOGIT_LIB} )
endif()

message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] systec [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] systec [BOOST_HEADERS:${BOOST_HEADERS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] systec [BOOST_LIBS:${BOOST_LIBS}]" )
link_directories( ${BOOST_PATH_LIBS} )

target_link_libraries ( ${SYSTEC_LIB_NAME} ${BOOST_LIBS} ${SYSTEC_LIB_FILE})
set_property(TARGET ${SYSTEC_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET ${SYSTEC_LIB_NAME} PROPERTY LINKER_LANGUAGE CXX)


