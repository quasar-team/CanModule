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
project( CanModule LANGUAGES C CXX  VERSION 2.0.21  )  
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule version= ${PROJECT_VERSION}" )
set (CMAKE_CXX_STANDARD 11)


# use ua toolkit trace. CanModule does NOT depend on the ua toolkit by default, so this is kept undefined. See below for expicit dealing
# with this.
# set (LOGIT_HAS_UATRACE true )

# vendor build behaviour:
# CANMODULE_BUILD_VENDORS OFF or not specified: only build mockup, do not build any vendor libs (default phony)
# CANMODULE_BUILD_VENDORS ON, nothing else: build mockup and all vendor libs (default all on)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_SYSTEC OFF: build mockup and all vendor libs except systec (drop systec)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_ANAGATE OFF: build mockup and all vendor libs except anagate (drop anagate)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_PEAK OFF: build mockup and all vendor libs except peak (drop peak)
# i.e. set(CANMODULE_BUILD_VENDORS ON )

# for linux, pull in as many static libs as possible. Vendor libs are not always fully static (relocatable), that is why we still end up with shared libs
# specify -DCANMODULE_AS_STATIC_AS_POSSIBLE (environment or here) to switch the CanLibLoaderLin lib names accordingly
# so that the "as-static-as-possible" shared libs are used by CanModule
#option(CANMODULE_AS_STATIC_AS_POSSIBLE "Build ${PROJECT_NAME} as-static-as-possible (default off, shared)" OFF )


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

# unsure if that works correctly
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

# unsure if that works correctly
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

# pull in LogIt as sources. Don't clone if dir exists already. 
function ( clone_LogIt ) 
  SET (LOGIT_VERSION "master")
  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: cloning LogIt from github. *NOTE* cloning version [${LOGIT_VERSION}]")
  if ( NOT EXISTS ${PROJECT_BINARY_DIR}/LogIt ) 
	  execute_process(COMMAND git clone -b ${LOGIT_VERSION} https://github.com/quasar-team/LogIt.git WORKING_DIRECTORY ${PROJECT_BINARY_DIR}  )
	  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt cloned from LogIt repo into ${PROJECT_BINARY_DIR}/LogIt")
   ELSE()
	  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Logit is already cloned into ${PROJECT_BINARY_DIR}/LogIt" )
   ENDIF() 
   set ( LOGIT_HEADERS ${PROJECT_BINARY_DIR}/LogIt/include ) # some bw compatibility, otherwise maybe useless
   include_directories( ${PROJECT_BINARY_DIR}/LogIt/include )
endfunction()


# LogIt options. If they are not set, set them to convenient defaults with minimum dependencies
# any options which exist are untouched (and left to the top-level project CMakeLists.txt)
# - default statc linking via source
# - default boost rotating log
IF ( NOT DEFINED LOGIT_BUILD_STAND_ALONE)
	option(STANDALONE_BUILD "Build ${PROJECT_NAME} as a stand-alone library (rather than a quasar optional module)" OFF )
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule default STANDALONE_BUILD= [${STANDALONE_BUILD}]")
	IF ( (NOT DEFINED LOGIT_BUILD_STATIC_LIB) AND (NOT DEFINED LOGIT_BUILD_SHARED_LIB) )
		option(LOGIT_BUILD_STATIC_LIB "Build ${PROJECT_NAME} static lib " ON )
		option(LOGIT_BUILD_SHARED_LIB "Build ${PROJECT_NAME} static lib " OFF )
		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule default LOGIT_BUILD_STATIC_LIB= [${LOGIT_BUILD_STATIC_LIB}]")
		message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule default LOGIT_BUILD_SHARED_LIB= [${LOGIT_BUILD_SHARED_LIB}]")
	ENDIF()
ENDIF()

IF ( (NOT DEFINED LOGIT_BACKEND_STDOUTLOG) AND (NOT DEFINED LOGIT_BACKEND_BOOSTLOG) AND (NOT DEFINED LOGIT_BACKEND_UATRACE))
	option(LOGIT_BACKEND_BOOSTLOG "Build ${PROJECT_NAME} with boost log " OFF )
	option(LOGIT_BACKEND_STDOUTLOG "Build ${PROJECT_NAME} stdout log is off " ON )
	option(LOGIT_BACKEND_UATRACE "Build ${PROJECT_NAME} ustrace log is off " OFF )
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule default LOGIT_BACKEND_BOOSTLOG= ${LOGIT_BACKEND_BOOSTLOG}") 
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule default LOGIT_BACKEND_STDOUTLOG= ${LOGIT_BACKEND_STDOUTLOG}") 
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule default LOGIT_BACKEND_UATRACE= ${LOGIT_BACKEND_UATRACE}") 
ENDIF()

# seems not needed any more
#set(LOGIT_BUILD_OPTION "LOGIT_AS_INT_SRC" CACHE STRING "LogIt is a mandatory dependency of CanModule. Inclusion options LOGIT_AS_INT_SRC, LOGIT_AS_EXT_SHARED, LOGIT_AS_EXT_STATIC")
#set_property(CACHE LOGIT_BUILD_OPTION PROPERTY STRINGS LOGIT_AS_INT_SRC LOGIT_AS_EXT_SHARED LOGIT_AS_EXT_STATIC)
#message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt build option LOGIT_BUILD_OPTION [${LOGIT_BUILD_OPTION}]")


# switch sources for the lib loader
IF (WIN32)
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL CanLibLoader/src/CanLibLoaderWin.cpp)
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL_HEADER CanLibLoader/include/CanLibLoaderWin.h)
ELSE()
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL CanLibLoader/src/CanLibLoaderLin.cpp)
	SET( CAN_LIBRARY_LOADER_PLATFORM_IMPL_HEADER CanLibLoader/include/CanLibLoaderLin.h)
	# use RUNPATH instead of RPATH -Wl,--disable-new-dtags does not work on g++ (GCC) 4.8.5 20150623 (Red Hat 4.8.5-39)
    SET( CMAKE_SKIP_RPATH TRUE )
	SET( CANMODULE_RPATHS "-Wl,-rpath,./ -Wl,-rpath,./lib -Wl,-rpath,/usr/local/lib" )
ENDIF()

# versioning burnt into the bins, but don't overwrite in the build if exists
if (NOT EXISTS ${PROJECT_SOURCE_DIR}/CanInterface/include/VERSION.h )
	file(WRITE ${PROJECT_SOURCE_DIR}/CanInterface/include/VERSION.h "// VERSION.h - do not edit\n#define CanModule_VERSION \"${PROJECT_VERSION}\"" )
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: generated VERSION.h into ${PROJECT_BINARY_DIR}/generated")
ELSE()
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule VERSION.h exists already in ${PROJECT_BINARY_DIR}/generated, don't touch it")
ENDIF()

file(GLOB_RECURSE SRCS_CPP ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE SRCS_H ${PROJECT_SOURCE_DIR}/*.h)
SET( SRCS ${SRCS_CPP} ${SRCS_H} )
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: SRCS= ${SRCS}")



SET( XXSRCS
		#Diagnostic/src/Diag.cpp
		#Diagnostic/include/Diag.h
		#CanLibLoader/src/CanLibLoader.cpp
		#CanLibLoader/include/CanLibLoader.h
		#${CAN_LIBRARY_LOADER_PLATFORM_IMPL}
		#${CAN_LIBRARY_LOADER_PLATFORM_IMPL_HEADER}		
		CanInterface/src/CanStatistics.cpp    
		#CanInterface/include/CanStatistics.h 
		CanInterface/src/CanModuleUtils.cpp
		#CanInterface/include/CanModuleUtils.h
		#CanInterface/src/CanBusAccess.cpp
		#CanInterface/include/CanBusAccess.h
		#CanInterface/include/CanMessage.h
		CanInterface/src/CCanAccess.cpp
		#CanInterface/include/CCanAccess.h
		${PROJECT_SOURCE_DIR}/CanInterface/include/VERSION.h	
)

include_directories( 
	include 
	CanLibLoader/include
	CanInterface/include 
	Diagnostic/include 	
)


# manage build options
# - static as possible
# - stand alone
IF ( ( DEFINED $ENV{CANMODULE_AS_STATIC_AS_POSSIBLE} ) OR ( DEFINED CANMODULE_AS_STATIC_AS_POSSIBLE ) )
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
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
    if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
  	    clone_LogIt()
        add_subdirectory( ${PROJECT_BINARY_DIR}/LogIt build )  
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
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_PATH_HEADERS:${BOOST_PATH_HEADERS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] [BOOST_LIBS:${BOOST_LIBS}]" )

target_link_libraries( ${CANMODULE_LIB_NAME} ${CANMODULE_RPATHS} )

# build implementation shared libs (reqd at runtime - loaded dynamically)
# we have a mock up build without any vendor libs as well, for CI where we don't want to distribute vendor libs
# preferably inject this from the toolchain
# if you have nothing declared, vendors will be OFF
# set ( CANMODULE_BUILD_VENDORS "ON" )
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: building vendor libs is ${BUILD_VENDORS}")
if ( NOT DEFINED CANMODULE_BUILD_VENDORS )
	set ( CANMODULE_BUILD_VENDORS "OFF" )
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] you have not specified if you want to build the vendor code or not ( CANMODULE_BUILD_VENDORS ). Set to OFF")
endif() 

add_subdirectory(CanInterfaceImplementations) 




