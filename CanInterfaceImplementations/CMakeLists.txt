cmake_minimum_required(VERSION 2.8)

IF (WIN32)	
	add_subdirectory(systec)
	add_subdirectory(pkcan)
	add_subdirectory(anagate)
	add_subdirectory(unitTestMockUpImplementation)
ELSE()	
	#sockcan does not work in centos7, as the library is not available, and anagate wont compile on sl6 because of the compiler version, please take this into account.
	#add_subdirectory(sockcan)
	add_subdirectory(unitTestMockUpImplementation)
	#add_subdirectory(anagate)
ENDIF()

message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: STANDALONE_BUILD [${STANDALONE_BUILD}] STANDALONE_BUILD_SHARED [${STANDALONE_BUILD_SHARED}]")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")

#add_library (CanInterfaceImplementations OBJECT)	
#set_target_properties(CanInterfaceImplementations PROPERTIES LINKER_LANGUAGE Cxx)
