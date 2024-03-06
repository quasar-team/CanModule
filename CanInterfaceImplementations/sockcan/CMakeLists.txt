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
project( sockcan LANGUAGES C CXX )

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib )

if( (NOT DEFINED SOCKETCAN_LIB_PATH) OR (NOT DEFINED SOCKETCAN_HEADERS) OR (NOT DEFINED SOCKETCAN_LIB_FILE)  )
	SET ( SOCKETCAN_LIB_PATH "/usr/local/lib" )
	SET ( SOCKETCAN_HEADERS "/usr/local/include" )
	SET ( SOCKETCAN_LIB_FILE -lsocketcan )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable SOCKETCAN_LIB_PATH has not been defined. assuming ${SOCKETCAN_PATH_LIBS} " )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable SOCKETCAN_HEADERS has not been defined. assuming ${SOCKETCAN_HEADERS} " )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable SOCKETCAN_LIB_FILE has not been defined. assuming ${SOCKETCAN_LIBS} " )
endif() 
include_directories( ${SOCKETCAN_HEADERS}  )
link_directories( ${SOCKETCAN_LIB_PATH} )


set(SOCKCAN_SOURCES
	ExecCommand.cpp
	UdevAnalyserForPeak.cpp
	SockCanScan.cpp
	../../CanInterface/src/CanStatistics.cpp	
	../../CanInterface/src/CanModuleUtils.cpp	
	../../CanInterface/src/CCanAccess.cpp	
	../../Diagnostic/src/Diag.cpp	
)

set(XXXSOCKCAN_SOURCES
	ExecCommand.cpp
	#ExecCommand.h
	UdevAnalyserForPeak.cpp
	#UdevAnalyserForPeak.h
	SockCanScan.cpp
	#SockCanScan.h
	CCanAccess.cpp
	#CCanAccess.h
	../../CanInterface/src/CanStatistics.cpp
	#../../CanInterface/include/CanStatistics.h	
)

# you can either build the fully dynamic libs or the as-static-as-possible, 
# but not both in one go, we profit and also link the linux socketcan static if specified
SET ( SOCK_LIB_NAME "sockcan" )
IF ( $ENV{CANMODULE_AS_STATIC_AS_POSSIBLE} )
	add_library( ${SOCK_LIB_NAME} STATIC ${SOCKCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
	target_link_libraries( ${SOCK_LIB_NAME} ${LOGIT_LIB} ${BOOST_LIBS} libsocketcan.a )
ELSE()
	add_library( ${SOCK_LIB_NAME} SHARED ${SOCKCAN_SOURCES} $<TARGET_OBJECTS:LogIt>)
	target_link_libraries( ${SOCK_LIB_NAME} ${LOGIT_LIB} ${BOOST_LIBS} -lsocketcan )
ENDIF()	

set_property(TARGET ${SOCK_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET ${SOCK_LIB_NAME} PROPERTY LINKER_LANGUAGE CXX)



