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

if(WIN32)
	SET(SYSTEC_INC_HINT "C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Include")
	SET(SYSTEC_LIB_HINT "C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Lib/USBCAN64.lib")
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: windows SYSTEC_LIB_HINT= ${SYSTEC_LIB_HINT}]")
else()
	SET(SYSTEC_INC_HINT "/some/path/to/include/directory/")
	SET(SYSTEC_LIB_HINT "/some/path/to/lib/binary")
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: linux SYSTEC_LIB_HINT= ${SYSTEC_LIB_HINT}]")
eendif()

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
