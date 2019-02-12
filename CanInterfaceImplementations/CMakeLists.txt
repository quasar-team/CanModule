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
project(CanInterfaceImplementations)

# we have a mock up build without any vendor libs as well, for CI where we don't want to distribute vendors
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] building vendors [CANMODULE_BUILD_VENDORS:${CANMODULE_BUILD_VENDORS}")

IF (WIN32)	
	IF(	CANMODULE_BUILD_VENDORS )
		add_subdirectory(systec)
		#add_subdirectory(pkcan)
		add_subdirectory(anagate)
	ENDIF()
ELSE()	
	IF(	CANMODULE_BUILD_VENDORS )
		add_subdirectory(sockcan)
		add_subdirectory(anagate)
	ENDIF()
	
ENDIF()
add_subdirectory(unitTestMockUpImplementation)

message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: [STANDALONE_BUILD:${STANDALONE_BUILD}]")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: [BOOST_HEADERS:${BOOST_HEADERS}]" )
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using boost libraries: [BOOST_LIBS:${BOOST_LIBS}]")  
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using LogIt library: [LOGIT_LIB:${LOGIT_LIB}]")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using [INCLUDE_DIRECTORIES:${INCLUDE_DIRECTORIES}]")



