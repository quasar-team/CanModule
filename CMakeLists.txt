# © Copyright CERN, Geneva, Switzerland, 2016.  All rights not expressly granted are reserved.
#
#   Created on: Wed Aug 30 10:15:57 CEST 2017
# 
#       Authors: Benjamin Farnham, Piotr Nikiel, Viacheslav Filimonov, Michael Ludwig <michael.ludwig@cern.ch>
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
#
cmake_minimum_required(VERSION 2.8)
project(CanModule)

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )  # static libs

SET( DEBUG "false" )
if( DEFINED DEBUG )
	message(STATUS "--WARNING-WARNING-WARNING-- generating DEBUG builds no matter what the command line says")
	IF (WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEBUG")
	ELSE()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -static")
	ENDIF()
else()
	message(STATUS "generating Release builds")
	IF (WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /RELEASE")
	ELSE()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
	ENDIF()
endif()

function ( locateBoostLibsUsingToolchainFile )
  if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message("CMAKE_TOOLCHAIN_FILE is defined -- including [[${CMAKE_TOOLCHAIN_FILE}]]")
    include(${CMAKE_TOOLCHAIN_FILE})    
  endif()
  if( NOT DEFINED BOOST_LIBS)
    message(FATAL_ERROR "Required variable BOOST_LIBS has not been defined. You must define a toolchain file describing (as a minimum) how boost headers/libs are resolved in the target build environment")
  endif()
  message(STATUS "function [locateBoostLibsUsingToolchainFile] located BOOST_LIBS [${BOOST_LIBS}]")
endfunction()

function ( locateLogItSharedLib )
	set(LOGIT_EXT_LIB_DIR "UNINIALIZED" CACHE PATH "Path to the directory containing the LogIt shared/static library binary file. Use absolute path, or relative to [${PROJECT_SOURCE_DIR}/]")
	set(LOGIT_INCLUDE_DIR "UNINIALIZED" CACHE PATH "Path to the directory containing the LogIt include files. Use absolute path, or relative to [${PROJECT_SOURCE_DIR}/]")
	message(STATUS "looking for LogIt... lib dir (-DLOGIT_EXT_LIB_DIR) [${LOGIT_EXT_LIB_DIR}] include dir (-DLOGIT_INCLUDE_DIR) [${LOGIT_INCLUDE_DIR}]")
	
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".lib" ".a") # windows looks for lib file to link to target dll: lib contains dll exports, symbols etc (see LogIt.h SHARED_LIB_EXPORT_DEFN)
	find_library( LOGITLIB NAMES LogIt PATHS ${LOGIT_EXT_LIB_DIR} NO_DEFAULT_PATH )
        
    if(NOT EXISTS ${LOGIT_INCLUDE_DIR})
        message(FATAL "No LogIt include directory found at [${LOGIT_INCLUDE_DIR}]. Please check the value of your -DLOGIT_INCLUDE_DIR build option" ) 
    endif()
    include_directories(${LOGIT_INCLUDE_DIR})

    set( LOGIT_LIB ${LOGITLIB} CACHE FILEPATH "location of the LogIt library the build will use")
    message(STATUS "Shared LogIt found, lib [${LOGIT_LIB}] headers [${LOGIT_INCLUDE_DIR}]")
endfunction()

function ( clone_LogIt )
  SET (LOGIT_VERSION "master")
  message(STATUS "cloning LogIt from github. *NOTE* cloning version [${LOGIT_VERSION}]")
  execute_process(COMMAND git clone -b ${LOGIT_VERSION} https://github.com/quasar-team/LogIt.git WORKING_DIRECTORY ${PROJECT_BINARY_DIR}  )
  include_directories( ${PROJECT_BINARY_DIR}/LogIt/include )
  set( LOGIT_HAS_UATRACE FALSE )
  message(STATUS "LogIt cloned from LogIt repo")
endfunction()

option(STANDALONE_BUILD "Build ${PROJECT_NAME} as a stand-alone library (rather than a quasar optional module)" OFF )
message(STATUS "STANDALONE_BUILD [${STANDALONE_BUILD}]")

#
# How does the stand-alone open62541-compat build want to use LogIt? Possible options:
# - Source
# - External shared
# - External static.
#
set(LOGIT_BUILD_OPTION "LOGIT_AS_INT_SRC" CACHE STRING "LogIt is a mandatory dependency of CanModule. Inclusion options LOGIT_AS_INT_SRC, LOGIT_AS_EXT_SHARED, LOGIT_AS_EXT_STATIC")
set_property(CACHE LOGIT_BUILD_OPTION PROPERTY STRINGS LOGIT_AS_INT_SRC LOGIT_AS_EXT_SHARED LOGIT_AS_EXT_STATIC)
message(STATUS "LogIt build option LOGIT_BUILD_OPTION [${LOGIT_BUILD_OPTION}]")
  
IF (WIN32)
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL CanLibLoader/src/CanLibLoaderWin.cpp)
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL_HEADER CanLibLoader/include/CanLibLoaderWin.h)
ELSE()
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL CanLibLoader/src/CanLibLoaderLin.cpp)
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL_HEADER CanLibLoader/include/CanLibLoaderLin.h)
	SET( CAN_LIBRARY_GETTIMEOFDAY )
	SET( CAN_LIBRARY_GETTIMEOFDAY_HEADER )
ENDIF()

SET( SRCS
		CanLibLoader/src/CanLibLoader.cpp
		CanLibLoader/include/CanLibLoader.h
		${CAN_LIBRARY_LOADER_PLATFORM_IMPL}
		${CAN_LIBRARY_LOADER_PLATFORM_IMPL_HEADER}		
		CanInterface/src/CanStatistics.cpp    
		CanInterface/include/CanStatistics.h 
		CanInterface/src/CanModuleUtils.cpp
		CanInterface/include/CanModuleUtils.h
		CanInterface/src/CanBusAccess.cpp
		CanInterface/include/CanBusAccess.h
		CanInterface/include/CanMessage.h
		CanInterface/include/CCanAccess.h
		${CAN_LIBRARY_GETTIMEOFDAY}
		${CAN_LIBRARY_GETTIMEOFDAY_HEADER}		
)

include_directories( include )
include_directories( CanLibLoader/include )
include_directories( CanInterface/include )


SET ( CANMODULE_LIB_NAME "CanModule-static" )

if(NOT STANDALONE_BUILD)
	add_library ( CanModule OBJECT ${SRCS} )
	set( SERVER_LINK_LIBRARIES ${SERVER_LINK_LIBRARIES} ${CMAKE_DL_LIBS} PARENT_SCOPE )
	message( STATUS "SERVER_LINK_LIBRARIES [${SERVER_LINK_LIBRARIES}]" )
else()
	#locateBoostLibsUsingToolchainFile()
	message(STATUS "Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
    
    if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
  	    clone_LogIt()
        add_subdirectory( ${PROJECT_BINARY_DIR}/LogIt )
        message(STATUS "LogIt added as compiled object code from sub-directory LogIt")
        add_library( ${CANMODULE_LIB_NAME} STATIC ${SRCS} $<TARGET_OBJECTS:LogIt> )
    else()
	    locateLogItSharedLib()
	    message(STATUS "Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")
	    add_library( ${CANMODULE_LIB_NAME} STATIC ${SRCS} )
	    target_link_libraries( ${CANMODULE_LIB_NAME}  ${LOGIT_LIB} )
    endif()
    target_link_libraries( ${CANMODULE_LIB_NAME} ${BOOST_LIBS} )
    
    add_subdirectory(CanModuleTest)
    # message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] disabled CanModuleTest" )
    	
endif()

 message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
 message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_HEADERS:${BOOST_HEADERS}]" )
 message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_LIBS:${BOOST_LIBS}]" )


# we have a mock up build without any vendor libs as well, for CI where we don't want to distribute vendor libs
# preferably inject this from the toolchain
# if you have nothing declared, vendors will be OFF
#set ( CANMODULE_BUILD_VENDORS "ON" )
message(STATUS " building vendor libs is ${BUILD_VENDORS}")
if ( NOT CANMODULE_BUILD_VENDORS )
	set ( CANMODULE_BUILD_VENDORS "OFF" )
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] you have not specified if you want to build the vendor code or not ( CANMODULE_BUILD_VENDORS ). Set to OFF")
endif() 

add_subdirectory(CanInterfaceImplementations) 




