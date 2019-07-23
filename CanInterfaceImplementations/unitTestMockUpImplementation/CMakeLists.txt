cmake_minimum_required(VERSION 2.8)
project(MockUpCanImplementationcan)

message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: STANDALONE_BUILD [${STANDALONE_BUILD}]")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

set(MOCK_SOURCES
	MockCanAccess.cpp
	MockCanAccess.h
	../../CanInterface/src/CanStatistics.cpp
	../../CanInterface/include/CanStatistics.h	
)

SET ( MOCKUP_LIB_NAME "MockUpCanImplementationcan" )

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( ${MOCKUP_LIB_NAME} SHARED ${MOCK_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( ${MOCKUP_LIB_NAME} SHARED ${MOCK_SOURCES} )
	target_link_libraries( ${MOCKUP_LIB_NAME} ${LOGIT_LIB} )
endif()
target_link_libraries ( ${MOCKUP_LIB_NAME}	${BOOST_LIBS} )

set_property(TARGET ${MOCKUP_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET ${MOCKUP_LIB_NAME} PROPERTY LINKER_LANGUAGE CXX)
