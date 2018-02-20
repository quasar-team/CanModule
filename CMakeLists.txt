cmake_minimum_required(VERSION 2.8)

function ( locateBoostLibsUsingToolchainFile )
  if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message("CMAKE_TOOLCHAIN_FILE is defined -- including [[${CMAKE_TOOLCHAIN_FILE}]]")
    include(${CMAKE_TOOLCHAIN_FILE})    
  endif()
  if( NOT DEFINED BOOST_LIBS)
    message(FATAL_ERROR "Required variable BOOST_LIBS has not been defined. You must define a toolchain file describing (as a minimum) how boost headers/libs are resolved in the target build environment")
  endif()   
endfunction()

function ( locateLogItSharedLib )
	set(LOGIT_EXT_LIB_DIR "UNINIALIZED" CACHE PATH "Path to the directory containing the LogIt shared/static library binary file. Use absolute path, or relative to [${PROJECT_SOURCE_DIR}/]")
	message(STATUS "Using LogIt build option [${LOGIT_BUILD_OPTION}]")
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".lib") # windows looks for lib file to link to target dll: lib contains dll exports, symbols etc (see LogIt.h SHARED_LIB_EXPORT_DEFN)
	find_library( LOGITLIB NAMES LogIt PATHS ${LOGIT_EXT_LIB_DIR} NO_DEFAULT_PATH )
    message(STATUS "LogIt added as external library dependency LOGITLIB [${LOGITLIB}]")
endfunction()

option(STANDALONE_BUILD "Build it as a stand-alone library instead of for Quasar" OFF )
option(STANDALONE_BUILD_SHARED "When building in stand-alone, build shared library rather than static library" OFF)
message(STATUS "STANDALONE_BUILD [${STANDALONE_BUILD}] STANDALONE_BUILD_SHARED [${STANDALONE_BUILD_SHARED}]")

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

add_subdirectory(CanInterfaceImplementations) #just to build the shared lib (reqd at runtime - loaded dynamically)
add_subdirectory(GoogleTest)
add_subdirectory(CanModuleTest)

#add_subdirectory(CanLibLoader)
#add_subdirectory(CanInterface)

if(NOT STANDALONE_BUILD)
	add_library ( CanModule OBJECT ${SRCS} )
else()
	locateBoostLibsUsingToolchainFile()
	message(STATUS "Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  

	locateLogItSharedLib()
	message(STATUS "Using LogIt library: LOGITLIB [${LOGITLIB}]")
	
	add_library( CanModule SHARED ${SRCS} )	
	target_link_libraries( CanModule ${BOOST_LIBS} ${LOGITLIB} )
endif()