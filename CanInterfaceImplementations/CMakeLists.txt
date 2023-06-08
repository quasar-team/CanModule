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
project( CanInterfaceImplementations LANGUAGES C CXX )
cmake_policy(SET CMP0054 NEW)

# we have a mock up build without any vendor libs as well, for CI where we don't want to distribute vendors
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] building vendors [CANMODULE_BUILD_VENDORS:${CANMODULE_BUILD_VENDORS}]")

# build behaviour:
# CANMODULE_BUILD_VENDORS OFF or not specified: only build mockup, do not build any vendor libs (default phony)
# CANMODULE_BUILD_VENDORS ON, nothing else: build mockup and all vendor libs (default all on)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_SYSTEC OFF: build mockup and all vendor libs except systec (drop systec)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_ANAGATE OFF: build mockup and all vendor libs except anagate (drop anagate)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_ANAGATE2 OFF: build mockup and all vendor libs except anagate2 (drop anagate2)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_PEAK OFF: build mockup and all vendor libs except peak (drop peak)
IF(	NOT DEFINED CANMODULE_BUILD_SYSTEC )
	SET(CANMODULE_BUILD_SYSTEC "ON")
ENDIF()
IF(	NOT DEFINED CANMODULE_BUILD_ANAGATE )
	SET(CANMODULE_BUILD_ANAGATE "ON")
ENDIF()
IF(	NOT DEFINED CANMODULE_BUILD_ANAGATE2 )
	SET(CANMODULE_BUILD_ANAGATE2 "ON")
ENDIF()
IF(	NOT DEFINED CANMODULE_BUILD_PEAK )
	SET(CANMODULE_BUILD_PEAK "ON")
ENDIF()

message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] build systec  [CANMODULE_BUILD_SYSTEC:${CANMODULE_BUILD_SYSTEC}]")
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] build anagate [CANMODULE_BUILD_ANAGATE:${CANMODULE_BUILD_ANAGATE}]")
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] build anagate2 [CANMODULE_BUILD_ANAGATE2:${CANMODULE_BUILD_ANAGATE2}]")
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] build peak    [CANMODULE_BUILD_PEAK:${CANMODULE_BUILD_PEAK}]")
IF (WIN32)	
	IF(	CANMODULE_BUILD_VENDORS STREQUAL "ON" )
		IF(	CANMODULE_BUILD_SYSTEC STREQUAL "ON" )
			add_subdirectory(systec)
			message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] building vendor systec for windows")
		ENDIF()
		IF(	CANMODULE_BUILD_ANAGATE STREQUAL "ON" )
			add_subdirectory(anagate)
			message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] building vendor anagate for windows")
		ENDIF()
		IF(	CANMODULE_BUILD_ANAGATE2 STREQUAL "ON" )
			add_subdirectory(anagate2)
			message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] building vendor anagate2 for windows")
		ENDIF()
		IF(	CANMODULE_BUILD_PEAK STREQUAL "ON" )
			add_subdirectory(pkcan)
			message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] building vendor peak for windows")
		ENDIF()
	ENDIF()
ELSE()	
	IF(	CANMODULE_BUILD_VENDORS STREQUAL "ON" )
		IF(	CANMODULE_BUILD_SYSTEC STREQUAL "ON" OR CANMODULE_BUILD_PEAK STREQUAL "ON" )
			add_subdirectory(sockcan) # systec, peak
			message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] building socketcan for vendors systec, peak for linux")
		ENDIF()
		IF(	CANMODULE_BUILD_ANAGATE STREQUAL "ON" )
			add_subdirectory(anagate)
			message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] building vendor anagate for linux")
		ENDIF()
		IF(	CANMODULE_BUILD_ANAGATE2 STREQUAL "ON" )
			add_subdirectory(anagate2)
			message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] building vendor anagate2 for linux")
		ENDIF()
	ENDIF()
ENDIF()
add_subdirectory(unitTestMockUpImplementation)

message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: [STANDALONE_BUILD:${STANDALONE_BUILD}]")
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: [BOOST_HEADERS:${BOOST_HEADERS}]" )
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using boost libraries: [BOOST_LIBS:${BOOST_LIBS}]")  
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using LogIt library: [LOGIT_LIB:${LOGIT_LIB}]")
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using [INCLUDE_DIRECTORIES:${INCLUDE_DIRECTORIES}]")

message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: PROJECT_BINARY_DIR= ${PROJECT_BINARY_DIR}]")



