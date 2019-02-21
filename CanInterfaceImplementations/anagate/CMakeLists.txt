# © Copyright CERN, Geneva, Switzerland, 2016.  All rights not expressly granted are reserved.
#
#   Created on: Wed Aug 30 10:15:57 CEST 2017
# 
#  		Authors: Michael Ludwig <michael.ludwig@cern.ch>
#       	Benjamin Farnham, Piotr Nikiel, Viacheslav Filimonov
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

# check if toolchain injection was correct for anagate
# anagate libs are not installed using a default installer/location. In fact these libs are 
# convenience libs to wrap the tcp specific telegrams needed for anagate into a nice API. 
# and this needs no driver install etc. Spec for anagate libs 
# is at http://www.anagate.de/download/Manual-AnaGate-TCPIP-V1.2.6-EN.pdf 
if( NOT DEFINED ANAGATE_PATH_LIBS ) 
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable ANAGATE_LIB_FILE has not been defined, take default." )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable ANAGATE_INC_DIR has not been defined, take default." )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable ANAGATE_PATH_LIBS has not been defined, take default." )
	if(WIN32)  
		if( 64BIT )  
			SET( ANAGATE_INC_DIR "C:/3rdPartySoftware/AnaGateCAN/win64/vc8/include")
			SET( ANAGATE_PATH_LIBS "C:/3rdPartySoftware/AnaGateCAN/win64/vc8/Release")
			SET( ANAGATE_LIB_FILE "C:/3rdPartySoftware/AnaGateCAN/win64/vc8/Release/AnaGateCanDll64.lib")
		else()
			SET( ANAGATE_INC_DIR "/3rdPartySoftware/AnaGateCAN/win32/vc8/include")
			SET( ANAGATE_PATH_LIBS "/3rdPartySoftware/AnaGateCAN/win32/vc8/Release")
			SET( ANAGATE_LIB_FILE "/3rdPartySoftware/AnaGateCAN/win32/vc8/Release/AnaGateCanDll.lib")
		endif()
	else()
		SET ( ANAGATE_PATH_LIBS "/opt/3rdPartySoftware/Anagate/CAN/anagate-api-2.13/linux64/gcc4_6" )
		SET ( ANAGATE_INC_DIR "/opt/3rdPartySoftware/Anagate/CAN/anagate-api-2.13/linux64/gcc4_6/include" )
		SET ( ANAGATE_LIB_FILE "-lAPIRelease64 -lCANDLLRelease64" )
	endif() 
ENDIF()

message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: ANAGATE_INC_DIR= ${ANAGATE_INC_DIR}]")
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: ANAGATE_PATH_LIBS= ${ANAGATE_PATH_LIBS}]")
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: ANAGATE_LIB_FILE= ${ANAGATE_LIB_FILE}]")
 

set(ANAGATE_SOURCES
	AnaCanScan.cpp
	AnaCanScan.h
	../../CanInterface/src/CanStatistics.cpp
	../../CanInterface/include/CanStatistics.h	
	../../CanInterface/src/CanModuleUtils.cpp
	../../CanInterface/include/CanModuleUtils.h	
)
	
link_directories( ${ANAGATE_PATH_LIBS} )
include_directories ( ${ANAGATE_INC_DIR} )
	
if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( ancan SHARED ${ANAGATE_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( ancan SHARED ${ANAGATE_SOURCES} )
	target_link_libraries( ancan ${LOGIT_LIB} )
endif()

#checkCANImplResourceExists(ANAGATE_LIB_FILE ${ANAGATE_LIB_HINT})
target_link_libraries ( ancan ${BOOST_LIBS}	${ANAGATE_LIB_FILE} )

set_property(TARGET ancan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET ancan PROPERTY LINKER_LANGUAGE CXX)

link_directories( ${BOOST_PATH_LIBS} )

