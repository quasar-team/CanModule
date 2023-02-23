# � Copyright CERN, Geneva, Switzerland, 2016.  All rights not expressly granted are reserved.
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
project( pkcan LANGUAGES C CXX )

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib )

# check if toolchain injection was correct for peak
# for windows we should use the default peak installer and location
# for linux we need libsocketcan
if( (NOT DEFINED PEAK_LIB_PATH) OR (NOT DEFINED PEAK_HEADERS) OR (NOT DEFINED PEAK_LIB_FILE)  )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable PEAK_HEADERS has not been defined." )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable PEAK_LIB_PATH has not been defined." )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable PEAK_LIB_FILE has not been defined." )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: assuming default locations and libs for peak and windows" )
	if(WIN32)
		#SET(PEAK_HEADERS "C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/Include")
		#SET(PEAK_LIB_PATH "C:/3rdPartySoftware/PeakCAN/pcan-basic/PCAN-Basic API/x64/VC_LIB")
		SET(PEAK_HEADERS "C:/3rdPartySoftware/PCAN-Basic API/17.nov.2017/Include")
		SET(PEAK_LIB_PATH "C:/3rdPartySoftware/PCAN-Basic API/17.nov.2017/x64/VC_LIB")
		#
		# latest
		# SET(PEAK_HEADERS "C:/3rdPartySoftware/PCAN-Basic API/16.july.2022/Include")
		# SET(PEAK_LIB_PATH "C:/3rdPartySoftware/PCAN-Basic API/16.july.2022/x64/VC_LIB")
		SET(PEAK_LIB_FILE "PCANBasic.lib")

		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: windows PEAK_LIB_PATH= ${PEAK_LIB_PATH}]")
 		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: windows PEAK_HEADERS=  ${PEAK_HEADERS}]")
 		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: windows PEAK_LIB_FILE= ${PEAK_LIB_FILE}]")
	else()
	    # since we use socketcan for linux for systec, we are in the wrong directory
		message( FATAL_ERROR "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: linux uses socketcan (directory sockcan) for peak, you are building the wrong directory." )
	endif()
else()
  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: toolchain PEAK_LIB_PATH= ${PEAK_LIB_PATH}" )
  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: toolchain PEAK_HEADERS=  ${PEAK_HEADERS}" )
  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: toolchain PEAK_LIB_FILE= ${PEAK_LIB_FILE}" )
endif() 
include_directories ( ${PEAK_HEADERS} )
link_directories ( ${PEAK_LIB_PATH} )

set(PEAKCAN_SOURCES
	pkcan.cpp
	../../CanInterface/src/CanStatistics.cpp	
	../../CanInterface/src/CanModuleUtils.cpp	
	../../CanInterface/src/CCanAccess.cpp	
)

SET ( PEAKCAN_LIB_NAME "pcan" )

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( ${PEAKCAN_LIB_NAME} SHARED ${PEAKCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( ${PEAKCAN_LIB_NAME} SHARED ${PEAKCAN_SOURCES} )
	target_link_libraries( pkcan ${LOGIT_LIB} )
endif()

target_link_libraries ( ${PEAKCAN_LIB_NAME} ${BOOST_LIBS} ${PEAK_LIB_FILE} )

set_property(TARGET ${PEAKCAN_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET ${PEAKCAN_LIB_NAME} PROPERTY LINKER_LANGUAGE CXX)

message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] peak [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] peak [BOOST_HEADERS:${BOOST_HEADERS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] peak [BOOST_LIBS:${BOOST_LIBS}]" )

link_directories( ${BOOST_PATH_LIBS} )
