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
project(pkcan)

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

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

include_directories ( ${PEAKCAN_INC_DIR} )

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( pkcan SHARED ${PEAKCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( pkcan SHARED ${PEAKCAN_SOURCES} )
	target_link_libraries( pkcan ${LOGIT_LIB} )
endif()

target_link_libraries ( pkcan ${BOOST_LIBS} ${PEAK_LIB_FILE} )

set_property(TARGET pkcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET pkcan PROPERTY LINKER_LANGUAGE CXX)

message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] peak [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] peak [BOOST_HEADERS:${BOOST_HEADERS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] peak [BOOST_LIBS:${BOOST_LIBS}]" )

link_directories( ${BOOST_PATH_LIBS} )
