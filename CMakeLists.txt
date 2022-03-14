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
cmake_minimum_required(VERSION 3.0)
project( CanModule LANGUAGES C CXX  VERSION 2.0.17  )  
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule version= ${PROJECT_VERSION}" )

# specify -DCANMODULE_AS_STATIC_AS_POSSIBLE for cmake to switch the CanLibLoaderLin lib names accordingly
# so that the "as-static-as-possible" shared libs are used by CanModule

# use ua toolkit trace. CanModule does NOT depend on the ua toolkit by default, so this is undefined, and therefore set to false further down. 
# set (LOGIT_HAS_UATRACE true )

# select the vendor libs you need
# set ( CANMODULE_BUILD_VENDORS "ON" )

set (CMAKE_CXX_STANDARD 11)

IF (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
ELSE()
	if( DEFINED DEBUG )
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -fPIC")
	ELSE()
		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: generating linux Release build")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fPIC")
	ENDIF()
ENDIF()

# I dont use that, but leave it in for comfort
function ( locateBoostLibsUsingToolchainFile )
  if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message("[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CMAKE_TOOLCHAIN_FILE is defined -- including [[${CMAKE_TOOLCHAIN_FILE}]]")
    include(${CMAKE_TOOLCHAIN_FILE})    
  endif()
  if( NOT DEFINED BOOST_LIBS)
    message(FATAL_ERROR "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Required variable BOOST_LIBS has not been defined. You must define a toolchain file describing (as a minimum) how boost headers/libs are resolved in the target build environment")
  endif()
  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: function [locateBoostLibsUsingToolchainFile] located BOOST_LIBS [${BOOST_LIBS}]")
endfunction()

# I dont use that, but leave it in for comfort
function ( locateLogItSharedLib )
	set(LOGIT_EXT_LIB_DIR "UNINIALIZED" CACHE PATH "Path to the directory containing the LogIt shared/static library binary file. Use absolute path, or relative to [${PROJECT_SOURCE_DIR}/]")
	set(LOGIT_INCLUDE_DIR "UNINIALIZED" CACHE PATH "Path to the directory containing the LogIt include files. Use absolute path, or relative to [${PROJECT_SOURCE_DIR}/]")
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: looking for LogIt... lib dir (-DLOGIT_EXT_LIB_DIR) [${LOGIT_EXT_LIB_DIR}] include dir (-DLOGIT_INCLUDE_DIR) [${LOGIT_INCLUDE_DIR}]")
	
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".lib") # windows looks for lib file to link to target dll: lib contains dll exports, symbols etc (see LogIt.h SHARED_LIB_EXPORT_DEFN)
	find_library( LOGITLIB NAMES LogIt PATHS ${LOGIT_EXT_LIB_DIR} NO_DEFAULT_PATH )
        
    if(NOT EXISTS ${LOGIT_INCLUDE_DIR})
        message(FATAL "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: No LogIt include directory found at [${LOGIT_INCLUDE_DIR}]. Please check the value of your -DLOGIT_INCLUDE_DIR build option" ) 
    endif()
    include_directories(${LOGIT_INCLUDE_DIR})

    set( LOGIT_LIB ${LOGITLIB} CACHE FILEPATH "location of the LogIt library the build will use")
    message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Shared LogIt found, lib [${LOGIT_LIB}] headers [${LOGIT_INCLUDE_DIR}]")
endfunction()

# clone into top-level source dir. Don't clone if dir exists already. Delete clone to recharge git branch/version.
function ( clone_LogIt )
  SET (LOGIT_VERSION "master")
  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: cloning LogIt from github. *NOTE* cloning version [${LOGIT_VERSION}]")
  if ( NOT EXISTS ${PROJECT_SOURCE_DIR}/LogIt ) 
	  execute_process(COMMAND git clone -b ${LOGIT_VERSION} https://github.com/quasar-team/LogIt.git WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}  )
      # you can define it, but if not, we switch it off
      if ( NOT DEFINED LOGIT_HAS_UATRACE )
	  	set( LOGIT_HAS_UATRACE FALSE )
        message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LOGIT_HAS_UATRACE was not defined, therefore switch it off LOGIT_HAS_UATRACE= ${LOGIT_HAS_UATRACE}" )
	  ENDIF()
	  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt cloned from LogIt repo into ${PROJECT_SOURCE_DIR}/LogIt")
   ELSE()
	  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Logit is already cloned into ${PROJECT_SOURCE_DIR}/LogIt" )
   ENDIF() 
   include_directories( ${PROJECT_SOURCE_DIR}/LogIt/include )
endfunction()

# what ?
# todo: set only if not set yet
option(STANDALONE_BUILD "Build ${PROJECT_NAME} as a stand-alone library (rather than a quasar optional module)" OFF )
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: STANDALONE_BUILD [${STANDALONE_BUILD}]")

#
# How does the stand-alone open62541-compat build want to use LogIt? Possible options:
# - Source
# - External shared
# - External static.
#
set(LOGIT_BUILD_OPTION "LOGIT_AS_INT_SRC" CACHE STRING "LogIt is a mandatory dependency of CanModule. Inclusion options LOGIT_AS_INT_SRC, LOGIT_AS_EXT_SHARED, LOGIT_AS_EXT_STATIC")
set_property(CACHE LOGIT_BUILD_OPTION PROPERTY STRINGS LOGIT_AS_INT_SRC LOGIT_AS_EXT_SHARED LOGIT_AS_EXT_STATIC)
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt build option LOGIT_BUILD_OPTION [${LOGIT_BUILD_OPTION}]")
 
# sources  
IF (WIN32)
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL CanLibLoader/src/CanLibLoaderWin.cpp)
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL_HEADER CanLibLoader/include/CanLibLoaderWin.h)
ELSE()
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL CanLibLoader/src/CanLibLoaderLin.cpp)
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL_HEADER CanLibLoader/include/CanLibLoaderLin.h)
	SET( CAN_LIBRARY_GETTIMEOFDAY )
	SET( CAN_LIBRARY_GETTIMEOFDAY_HEADER )
	# use RUNPATH instead of RPATH -Wl,--disable-new-dtags does not work on g++ (GCC) 4.8.5 20150623 (Red Hat 4.8.5-39)
    SET( CMAKE_SKIP_RPATH TRUE )
	SET( CANMODULE_RPATHS "-Wl,-rpath,./ -Wl,-rpath,./lib -Wl,-rpath,/usr/local/lib" )
ENDIF()

# versioning burnt into the bins
if (NOT EXISTS ${PROJECT_BINARY_DIR}/generated/VERSION.h )
	file(WRITE ${PROJECT_BINARY_DIR}/generated/VERSION.h "// VERSION.h - do not edit\n#define CanModule_VERSION \"${PROJECT_VERSION}\"" )
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: generated VERSION.h into ${PROJECT_BINARY_DIR}/generated")
ELSE()
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule VERSION.h exists already in ${PROJECT_BINARY_DIR}/generated, don't touch it")
ENDIF()

SET( SRCS
		Diagnostic/src/Diag.cpp
		Diagnostic/include/Diag.h
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
		CanInterface/src/CCanAccess.cpp
		CanInterface/include/CCanAccess.h
		${CAN_LIBRARY_GETTIMEOFDAY}
		${CAN_LIBRARY_GETTIMEOFDAY_HEADER}	
		${PROJECT_BINARY_DIR}/generated/VERSION.h	
)

include_directories( 
	include 
	CanLibLoader/include
	CanInterface/include 
	Diagnostic/include 
	${PROJECT_BINARY_DIR}/generated
)


# manage build options
# - static as possible
# - stand alone
#
IF ( $ENV{CANMODULE_AS_STATIC_AS_POSSIBLE} )
	SET ( CANMODULE_LIB_NAME CanModule-static )
  	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: building static CanModule lib: ${CANMODULE_LIB_NAME}" )
ELSE()
	SET ( CANMODULE_LIB_NAME CanModule )
  	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: building shared CanModule lib: ${CANMODULE_LIB_NAME}" )
ENDIF()

if(NOT STANDALONE_BUILD)
	add_library ( ${CANMODULE_LIB_NAME} OBJECT ${SRCS} )
	set( SERVER_LINK_LIBRARIES ${SERVER_LINK_LIBRARIES} ${CMAKE_DL_LIBS} PARENT_SCOPE )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: SERVER_LINK_LIBRARIES= [${SERVER_LINK_LIBRARIES}]" )
else()
	#locateBoostLibsUsingToolchainFile()
	message(STATUS "Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
    if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
  	    clone_LogIt()
        add_subdirectory( ${PROJECT_SOURCE_DIR}/LogIt )
        message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt compiled object code from src sub-directory added")
        add_library( ${CANMODULE_LIB_NAME} SHARED ${SRCS} $<TARGET_OBJECTS:LogIt> )
    else()
	    locateLogItSharedLib()
	    message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")
	    add_library( ${CANMODULE_LIB_NAME} SHARED ${SRCS} )
	    target_link_libraries( ${CANMODULE_LIB_NAME} ${LOGIT_LIB} )
    endif()
    target_link_libraries( ${CANMODULE_LIB_NAME} ${BOOST_LIBS} )
endif()

# need boost signal/slot and timer
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_HEADERS:${BOOST_HEADERS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_LIBS:${BOOST_LIBS}]" )

# add rpath: burn some execution paths into bins for linux
#IF (WIN32)
#ELSE()
#    # use RUNPATH instead of RPATH -Wl,--disable-new-dtags does not work on g++ (GCC) 4.8.5 20150623 (Red Hat 4.8.5-39)
#    SET( CMAKE_SKIP_RPATH TRUE )
#	SET( CANMODULE_RPATHS "-Wl,-rpath,./ -Wl,-rpath,./lib -Wl,-rpath,/usr/local/lib" )
#ENDIF()

target_link_libraries( ${CANMODULE_LIB_NAME} ${CANMODULE_RPATHS} )

# build implementation shared libs (reqd at runtime - loaded dynamically)
# we have a mock up build without any vendor libs as well, for CI where we don't want to distribute vendor libs
# preferably inject this from the toolchain
# if you have nothing declared, vendors will be OFF
# set ( CANMODULE_BUILD_VENDORS "ON" )
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: building vendor libs is ${BUILD_VENDORS}")
if ( NOT CANMODULE_BUILD_VENDORS )
	set ( CANMODULE_BUILD_VENDORS "OFF" )
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] you have not specified if you want to build the vendor code or not ( CANMODULE_BUILD_VENDORS ). Set to OFF")
endif() 

add_subdirectory(CanInterfaceImplementations) 




