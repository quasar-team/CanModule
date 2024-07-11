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
cmake_minimum_required(VERSION 3.0)

project( ancan LANGUAGES C CXX )

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# check if toolchain injection was correct for anagate
# anagate libs are not installed using a default installer/location. In fact these libs are 
# convenience libs to wrap the tcp specific telegrams needed for anagate into a nice API. 
# and this needs no driver install etc. Spec for anagate libs 
# is at http://www.anagate.de/download/Manual-AnaGate-TCPIP-V1.2.6-EN.pdf 
if( (NOT DEFINED ANAGATE_LIB_PATH) OR (NOT DEFINED ANAGATE_HEADERS) OR (NOT DEFINED ANAGATE_LIB_FILE)  )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: at least one of the required variables ANAGATE_LIB_PATH, ANAGATE_HEADERS, ANAGATE_LIB_FILE has not been defined." )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: assuming default locations and libs for anagate (linux, windows)" )
	message(WARNING "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: since anagate does not have an installer the default locations are likely to be wrong" )
	if(WIN32)  
		SET( ANAGATE_HEADERS "C:/3rdPartySoftware/CAN/windows/anagate/beta.v6/include")
		SET( ANAGATE_LIB_PATH "C:/3rdPartySoftware/CAN/windows/anagate/beta.v6/lib")
		SET( ANAGATE_LIB_FILE "C:/3rdPartySoftware/CAN/windows/anagate/beta.v6/AnaGateCanDll64.lib")
	else()
		SET ( ANAGATE_HEADERS "/opt/3rdPartySoftware/CAN/cal9/anagate/beta.v6/include" )
		SET ( ANAGATE_LIB_PATH "/opt/3rdPartySoftware/CAN/cal9/anagate/beta.v6/lib" )
		SET ( ANAGATE_LIB_FILE "-lAnaGateExtStaticRelease -lAnaGateStaticRelease -lCANDLLStaticRelease64" )
	endif() 
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: ANAGATE_HEADERS=  ${ANAGATE_HEADERS}]")
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: ANAGATE_LIB_PATH= ${ANAGATE_LIB_PATH}]")
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: ANAGATE_LIB_FILE= ${ANAGATE_LIB_FILE}]")
else()
  	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: toolchain ANAGATE_HEADERS=  ${ANAGATE_HEADERS}" )
  	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: toolchain ANAGATE_LIB_PATH= ${ANAGATE_LIB_PATH}" )
  	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: toolchain ANAGATE_LIB_FILE= ${ANAGATE_LIB_FILE}" )
endif() 
link_directories( ${ANAGATE2_LIB_PATH} )
include_directories ( ${ANAGATE2_HEADERS} )

set(ANAGATE2_SOURCES
	AnaCanScan2.cpp
	../../CanInterface/src/CanStatistics.cpp	
	../../CanInterface/src/CanModuleUtils.cpp	
	../../CanInterface/src/CCanAccess.cpp	
	../../Diagnostic/src/Diag.cpp	
)

# you can either build the fully dynamic libs or the as-static-as-possible, 
# but not both in one go
IF ( $ENV{CANMODULE_AS_STATIC_AS_POSSIBLE} )
	SET ( CANMODULE_LINKAGE STATIC )

ELSE()
	SET ( CANMODULE_LINKAGE SHARED )
ENDIF()	
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CANMODULE_LINKAGE=  ${CANMODULE_LINKAGE}" )
SET ( ANAGATE2_LIB_NAME "an2can" )
add_library( ${ANAGATE2_LIB_NAME} ${CANMODULE_LINKAGE} ${ANAGATE2_SOURCES} $<TARGET_OBJECTS:LogIt>)
	
target_link_libraries ( ${ANAGATE2_LIB_NAME}  ${BOOST_LIBS}	${ANAGATE2_LIB_FILE} )
set_property(TARGET ${ANAGATE2_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET ${ANAGATE2_LIB_NAME} PROPERTY LINKER_LANGUAGE CXX)
link_directories( ${BOOST_PATH_LIBS} )

