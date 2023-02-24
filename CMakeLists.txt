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
cmake_minimum_required( VERSION 3.0 )
project( CanModuleProject LANGUAGES C CXX  VERSION 2.0.24  )  
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule version= ${PROJECT_VERSION}" )

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0002 NEW)

SET( CMAKE_VERBOSE_MAKEFILE ON )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib ) # the static lib is just thrown into build, a cmake bug, probably
SET( CMAKE_COLOR_MAKEFILE ON )
SET( CMAKE_CXX_STANDARD 11 ) 
SET( CMAKE_CXX_STANDARD_REQUIRED ON )
SET( CMAKE_POSITION_INDEPENDENT_CODE TRUE )
SET( CMAKE_BUILD_TYPE Release )

# -- FUNCTION SECTION START ---
#
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
  	  execute_process(COMMAND mkdir -p ${PROJECT_BINARY_DIR} ) 
  	  execute_process(COMMAND pwd ) 
	  execute_process(COMMAND git clone -b ${LOGIT_VERSION} https://github.com/quasar-team/LogIt.git WORKING_DIRECTORY ${PROJECT_BINARY_DIR}  )
	  
	  #execute_process(COMMAND git clone -b ${LOGIT_VERSION} https://github.com/quasar-team/LogIt.git WORKING_DIRECTORY ./build  )
	  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: PROJECT_BINARY_DIR= ${PROJECT_BINARY_DIR}")
	  #message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt cloned into ${PROJECT_BINARY_DIR}/../LogIt")
	  #message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt cloned into ./build/LogIt")
   ELSE()
	  message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Logit is already cloned into ${PROJECT_BINARY_DIR}/LogIt" )
   ENDIF() 
   set ( LOGIT_HEADERS ${PROJECT_BINARY_DIR}/LogIt/include ) # some bw compatibility, otherwise maybe useless
   include_directories( ${PROJECT_BINARY_DIR}/LogIt/include )
   #add_subdirectory( ${PROJECT_BINARY_DIR}/LogIt ${PROJECT_BINARY_DIR}/LogIt/build )
   add_subdirectory( ${PROJECT_BINARY_DIR}/LogIt ${PROJECT_BINARY_DIR}/build/LogIt )
   
endfunction()

#
# -- FUNCTION SECTTION END --

# use env var for static-as-possible or shared linking for linux (only shared for windows)
IF ( $ENV{CANMODULE_AS_STATIC_AS_POSSIBLE} MATCHES "true" )
	SET ( CANMODULE_AS_STATIC_AS_POSSIBLE TRUE )
ELSE()
	SET ( CANMODULE_AS_STATIC_AS_POSSIBLE FALSE )
ENDIF()

IF ( ${CANMODULE_AS_STATIC_AS_POSSIBLE} )
	SET ( CANMODULE_LIB_NAME CanModule-static ) # expands to libCanModule-static.a
	SET ( LINK_OPTION STATIC )
	SET ( BUILD_SHARED_LIBS OFF )
ELSE()
	SET ( CANMODULE_LIB_NAME CanModule ) # expands to libCanModule.so
	SET ( LINK_OPTION SHARED )
	SET ( BUILD_SHARED_LIBS ON )
ENDIF()



# LogIt pull-in
if( STANDALONE_BUILD )
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt for standalone build  ]")  
    if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
  	    clone_LogIt()  # clones into ${PROJECT_SOURCE_DIR}/build/build/CanModule/LogIt
        message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] PROJECT_BINARY_DIR= ${PROJECT_BINARY_DIR}")          
        add_subdirectory( ${PROJECT_BINARY_DIR}/LogIt build ) 
    else()
	    locateLogItSharedLib()
	    message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")
    endif()
ENDIF()

#include_directories( CanInterface/include CanLibLoader/include Diagnostic/include CanModuleTest/include )
include_directories( CanInterface/include CanLibLoader/include Diagnostic/include )
include_directories ( ${BOOST_PATH_HEADERS} )
link_directories( ${BOOST_PATH_LIBS} )

# we have a mock up build without any vendor libs as well, for CI where we don't want to distribute vendor libs
# preferably inject this from the toolchain : set ( CANMODULE_BUILD_VENDORS ON )
# if you have nothing declared, vendors will be OFF
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: building vendor libs is ${BUILD_VENDORS}")
if ( NOT DEFINED CANMODULE_BUILD_VENDORS )
	set ( CANMODULE_BUILD_VENDORS OFF )
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] you have not specified if you want to build the vendor code or not ( CANMODULE_BUILD_VENDORS ). Set to OFF")
endif() 

# vendor build behaviour from the toolchain
# CANMODULE_BUILD_VENDORS OFF or not specified: only build mockup, do not build any vendor libs (default phony)
# CANMODULE_BUILD_VENDORS ON, nothing else: build mockup and all vendor libs (default all on)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_SYSTEC OFF: build mockup and all vendor libs except systec (drop systec)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_ANAGATE OFF: build mockup and all vendor libs except anagate (drop anagate)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_PEAK OFF: build mockup and all vendor libs except peak (drop peak)
# i.e. set(CANMODULE_BUILD_VENDORS ON )

# compiler/build flags
IF (WIN32)
  	message( STATUS " ${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Windows build" )
	add_definitions(-DSUPPORT_XML_CONFIG -W0 -DWIN32_LEAN_AND_MEAN )
	add_definitions(-D_WIN32_WINDOWS)
	add_definitions("/EHsc")
	
	# dynamic linking, no static yet available. Wait for v3/anagate 2023
	SET(CMAKE_CXX_FLAGS_RELEASE "/MD")
	SET(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi")
	
	SET( CMAKE_EXE_LINKER_FLAGS     "${CMAKE_EXE_LINKER_FLAGS}    /MACHINE:X64" )
	SET( CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} /MACHINE:X64" )
	SET( CMAKE_STATIC_LINKER_FLAGS  "${CMAKE_STATIC_LINKER_FLAGS} /MACHINE:X64" )
	SET( CMAKE_MODULE_LINKER_FLAGS  "${CMAKE_MODULE_LINKER_FLAGS} /MACHINE:X64" )
	
	find_package(Threads REQUIRED)
	#set ( SYSLIBS ${LOGIT_DEPENDENCIES} ${CMAKE_THREAD_LIBS_INIT} )
ELSE()
  	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Linux build" )
	IF ( ${CANMODULE_AS_STATIC_AS_POSSIBLE} )
		add_definitions( -Wall -Wno-deprecated -std=c++11 -Wno-literal-suffix -D_CERN_CC7 -O3  )
 	ELSE()
		add_definitions( -Wall -Wno-deprecated -std=c++11 -Wno-literal-suffix -D_CERN_CC7 -O3 -DBOOST_LOG_DYN_LINK )
	ENDIF()	
ENDIF()




# LogIt options. If they are not set, set them to convenient defaults with minimum dependencies
# any options which exist are untouched (and left to the top-level project CMakeLists.txt)
# - default static linking via source
# - default boost rotating log
# default no uatrace
# usage of the ua toolkit trace: CanModule itself does NOT depend on the ua toolkit of course, but LogIt might. See below for expicit dealing
# with this.
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
	# LOGIT_BUILD_STATIC_LIB OFF
	# LOGIT_BUILD_SHARED_LIB ON
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule default LOGIT_BACKEND_BOOSTLOG= ${LOGIT_BACKEND_BOOSTLOG}") 
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule default LOGIT_BACKEND_STDOUTLOG= ${LOGIT_BACKEND_STDOUTLOG}") 
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule default LOGIT_BACKEND_UATRACE= ${LOGIT_BACKEND_UATRACE}") 
ENDIF()

# switch sources according to OS. testing sources OFF, todo later
SET ( SRCS_ALL_OS 
	./CanInterface/src/CCanAccess.cpp
	./CanInterface/src/CanBusAccess.cpp
	./CanInterface/src/CanModuleUtils.cpp
	./CanInterface/src/CanStatistics.cpp
	./CanLibLoader/src/CanLibLoader.cpp
	#./CanModuleTest/src/CanModuleTest.cpp
	#./CanModuleTest/src/main.cpp
	./Diagnostic/src/Diag.cpp
)
SET ( SRCS_OS_WIN
	./CanLibLoader/src/CanLibLoaderWin.cpp
)
SET ( SRCS_OS_LIN 
	./CanLibLoader/src/CanLibLoaderLin.cpp
)

IF (WIN32)
	SET ( SRCS ${SRCS_ALL_OS} ${SRCS_OS_WIN}  )
ELSE()
	SET ( SRCS ${SRCS_ALL_OS} ${SRCS_OS_LIN}  )
	# use RUNPATH instead of RPATH -Wl,--disable-new-dtags does not work on g++ (GCC) 4.8.5 20150623 (Red Hat 4.8.5-39)
	# and we do not want runtime path info in the bins
    SET( CMAKE_SKIP_RPATH TRUE )
    SET( CMAKE_SKIP_INSTALL_RPATH TRUE )
	SET( CANMODULE_RPATHS "-Wl,-rpath,./ -Wl,-rpath,./lib -Wl,-rpath,/usr/local/lib" )
ENDIF()

# versioning burnt into the bins, but don't overwrite in the build if exists
if (NOT EXISTS ${PROJECT_SOURCE_DIR}/CanInterface/include/VERSION.h )
	file(WRITE ${PROJECT_SOURCE_DIR}/CanInterface/include/VERSION.h "// VERSION.h - do not edit\n#define CanModule_VERSION \"${PROJECT_VERSION}\"" )
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: generated VERSION.h into ${PROJECT_SOURCE_DIR}/CanInterface/include/")
ELSE()
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CanModule VERSION.h exists already in ${PROJECT_SOURCE_DIR}/CanInterface/include/, don't touch it")
ENDIF()


# here we define the main lib to build
add_library( ${CANMODULE_LIB_NAME} ${LINK_OPTION} ${SRCS} )

# manage LogIt build options
# LogIt as source or shared lib, standalone or quasar
if( NOT STANDALONE_BUILD )
	# add_library( ${TARGET_NAME} ${LINK_OPTION} ${SRCS} )
	set( SERVER_LINK_LIBRARIES ${SERVER_LINK_LIBRARIES} ${CMAKE_DL_LIBS} PARENT_SCOPE )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: SERVER_LINK_LIBRARIES= [${SERVER_LINK_LIBRARIES}]" )
else()
	#locateBoostLibsUsingToolchainFile()
	message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
    if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
        target_link_libraries( ${CANMODULE_LIB_NAME} $<TARGET_OBJECTS:LogIt> )
    else()
	    target_link_libraries( ${CANMODULE_LIB_NAME} ${LOGIT_LIB} )
    endif()
    target_link_libraries( ${CANMODULE_LIB_NAME} ${BOOST_LIBS} )
endif()

add_subdirectory(CanInterfaceImplementations) # all the subdirs for the vendors are underneath
 
# specify the RPATH skipping as well
target_link_libraries( ${CANMODULE_LIB_NAME} ${CANMODULE_RPATHS} )




