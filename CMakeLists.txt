cmake_minimum_required(VERSION 2.8)
project(CanModule)

# C++11 as standard
set (CMAKE_CXX_STANDARD 11) # works for CMake v3.1+
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11") #for older CMake versions

message(STATUS "--WARNING-WARNING-WARNING-- generating DEBUG builds no matter what the command line says")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
message(STATUS "--WARNING-WARNING-WARNING-- generating DEBUG builds no matter what the command line says")

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
	set(LOGIT_EXT_INC_DIR "UNINIALIZED" CACHE PATH "Path to the directory containing the LogIt include files. Use absolute path, or relative to [${PROJECT_SOURCE_DIR}/]")
	message(STATUS "looking for LogIt... lib dir (-DLOGIT_EXT_LIB_DIR) [${LOGIT_EXT_LIB_DIR}] include dir (-DLOGIT_EXT_INC_DIR) [${LOGIT_EXT_INC_DIR}]")
	
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".lib") # windows looks for lib file to link to target dll: lib contains dll exports, symbols etc (see LogIt.h SHARED_LIB_EXPORT_DEFN)
	find_library( LOGITLIB NAMES LogIt PATHS ${LOGIT_EXT_LIB_DIR} NO_DEFAULT_PATH )
        
    if(NOT EXISTS ${LOGIT_EXT_INC_DIR})
        message(FATAL "No LogIt include directory found at [${LOGIT_EXT_INC_DIR}]. Please check the value of your -DLOGIT_EXT_INC_DIR build option" ) 
    endif()
    include_directories(${LOGIT_EXT_INC_DIR})

    set( LOGIT_LIB ${LOGITLIB} CACHE FILEPATH "location of the LogIt library the build will use")
    message(STATUS "Shared LogIt found, lib [${LOGIT_LIB}] headers [${LOGIT_EXT_INC_DIR}]")
endfunction()

function ( clone_LogIt )
  SET (LOGIT_VERSION "master")
  message(STATUS "cloning LogIt from github. *NOTE* cloning version [${LOGIT_VERSION}]")
  execute_process(COMMAND git clone -b ${LOGIT_VERSION} https://github.com/quasar-team/LogIt.git WORKING_DIRECTORY ${PROJECT_BINARY_DIR}  )
  include_directories( ${CMAKE_SOURCE_DIR}/LogIt/include )
  set( LOGIT_HAS_UATRACE FALSE )
  message(STATUS "LogIt cloned from LogIt repo")
endfunction()

option(STANDALONE_BUILD "Build it as a stand-alone library instead of for Quasar" OFF )
option(STANDALONE_BUILD_SHARED "When building in stand-alone, build shared library rather than static library" OFF)
message(STATUS "STANDALONE_BUILD [${STANDALONE_BUILD}] STANDALONE_BUILD_SHARED [${STANDALONE_BUILD_SHARED}]")

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
	SET( CAN_LIBRARY_GETTIMEOFDAY CanInterface/src/gettimeofday.cpp)
	SET( CAN_LIBRARY_GETTIMEOFDAY_HEADER CanInterface/include/gettimeofday.h)
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
		CanInterface/src/CanLibExternCWrapper.cpp
		CanInterface/src/CanStatistics.cpp    
		CanInterface/src/CanModuleUtils.cpp
		CanInterface/src/MessageSharedQueue.cpp
		CanInterface/include/CanLibExternCWrapper.h
		CanInterface/include/CanStatistics.h 
		CanInterface/include/CanModuleUtils.h
		CanInterface/include/CanMessage.h
		CanInterface/include/CCanAccess.h
		CanInterface/include/MessageSharedQueue.h
		${CAN_LIBRARY_GETTIMEOFDAY}
		${CAN_LIBRARY_GETTIMEOFDAY_HEADER}		
)

include_directories( include )
include_directories( CanLibLoader/include )
include_directories( CanInterface/include )

#add_subdirectory(CanLibLoader)
#add_subdirectory(CanInterface)

if(NOT STANDALONE_BUILD)
	add_library ( CanModule OBJECT ${SRCS} )
else()
	locateBoostLibsUsingToolchainFile()
	message(STATUS "Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
    
    if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
  	    clone_LogIt()
        add_subdirectory( ${PROJECT_SOURCE_DIR}/LogIt )
        message(STATUS "LogIt added as compiled object code from sub-directory LogIt")
        add_library( CanModule SHARED ${SRCS} $<TARGET_OBJECTS:LogIt> )
    else()
	    locateLogItSharedLib()
	    message(STATUS "Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")
	    add_library( CanModule SHARED ${SRCS} )
	    target_link_libraries( CanModule ${LOGIT_LIB} )
    endif()
    target_link_libraries( CanModule ${BOOST_LIBS} )
    
    add_subdirectory(CanInterfaceImplementations) #just to build the shared lib (reqd at runtime - loaded dynamically)
    add_subdirectory(GoogleTest)
    include_directories( GoogleTest/googletest/googletest/include )
    add_subdirectory(CanModuleTest)
    	
endif()