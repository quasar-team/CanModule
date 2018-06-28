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
project(ancan)

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

if(WIN32)  
	SET(ANAGATE_INC_HINT "C:/3rdPartySoftware/AnaGateCAN/win64/vc8/include")
	SET(ANAGATE_LIB_HINT "C:/3rdPartySoftware/AnaGateCAN/win64/vc8/Release/AnaGateCanDll64.lib")
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: windows ANAGATE_INC_HINT= ${ANAGATE_INC_HINT}]")
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: windows ANAGATE_INC_HINT= ${ANAGATE_INC_HINT}]")
else()
	SET(ANAGATE_INC_HINT "/3rdPartySoftware/Anagate/anagate-api-2.13/linux64/gcc4_6/include/")
	SET(ANAGATE_LIB_HINT "/3rdPartySoftware/Anagate/anagate-api-2.13/linux64/gcc4_6/libAPIRelease64.so")
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: linux ANAGATE_INC_HINT= ${ANAGATE_INC_HINT}]")
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: linux ANAGATE_INC_HINT= ${ANAGATE_INC_HINT}]")
endif()

set(ANAGATE_SOURCES
	AnaCanScan.cpp
	AnaCanScan.h
	../../CanInterface/src/CanStatistics.cpp
	../../CanInterface/include/CanStatistics.h	
	../../CanInterface/src/CanModuleUtils.cpp
	../../CanInterface/include/CanModuleUtils.h	
)

checkCANImplResourceExists(ANAGATE_INC_DIR ${ANAGATE_INC_HINT})
include_directories ( ${ANAGATE_INC_DIR} )

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( ancan SHARED ${ANAGATE_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( ancan SHARED ${ANAGATE_SOURCES} )
	target_link_libraries( ancan ${LOGIT_LIB} )
endif()

checkCANImplResourceExists(ANAGATE_LIB_FILE ${ANAGATE_LIB_HINT})
target_link_libraries ( ancan ${BOOST_LIBS}	${ANAGATE_LIB_FILE} )
set_property(TARGET ancan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET ancan PROPERTY LINKER_LANGUAGE CXX)
