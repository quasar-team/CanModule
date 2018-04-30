cmake_minimum_required(VERSION 2.8)
project(CanInterfaceImplementations)

function ( checkCANImplResourceExists IMPL_LIB_RESOURCE HINT)
	if(EXISTS ${${IMPL_LIB_RESOURCE}})
		message(STATUS "OK: CAN implementation library resource [${IMPL_LIB_RESOURCE}=${${IMPL_LIB_RESOURCE}}] exists")	
	else()
		message(FATAL "ERROR: CAN implementation library resource [${IMPL_LIB_RESOURCE}=${${IMPL_LIB_RESOURCE}}] was not found. 
		CAN implementation library will *NOT* be built.
		Please specify the location of an existing file/directory via the cmake invocation (as per the following example):
		-D${IMPL_LIB_RESOURCE}=\"${HINT}\"")	
	endif()
endfunction()

IF (WIN32)	
	add_subdirectory(systec)
	add_subdirectory(pkcan)
	add_subdirectory(anagate)
	add_subdirectory(unitTestMockUpImplementation)
ELSE()	
	#sockcan does not work in centos7, as the library is not available, and anagate wont compile on sl6 because of the compiler version, please take this into account.
	add_subdirectory(sockcan)
	add_subdirectory(unitTestMockUpImplementation)
	#add_subdirectory(anagate)
ENDIF()

message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: STANDALONE_BUILD [${STANDALONE_BUILD}]")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using INCLUDE_DIRECTORIES: [${INCLUDE_DIRECTORIES}]")
