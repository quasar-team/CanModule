cmake_minimum_required(VERSION 3.1 )
project( MockUpCanImplementationcan LANGUAGES C CXX  VERSION 1.1.9.0  )

message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: STANDALONE_BUILD [${STANDALONE_BUILD}]")
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib )

set(MOCK_SOURCES
	MockCanAccess.cpp
	MockCanAccess.h
	../../CanInterface/src/CanStatistics.cpp
	../../CanInterface/include/CanStatistics.h	
)

IF ( DEFINED CANMODULE_AS_STATIC_AS_POSSIBLE )
	add_library( MockUpCanImplementationcan STATIC ${MOCK_SOURCES} $<TARGET_OBJECTS:LogIt>)
ELSE()
	add_library( MockUpCanImplementationcan SHARED ${MOCK_SOURCES} $<TARGET_OBJECTS:LogIt>)
ENDIF()

target_link_libraries ( MockUpCanImplementationcan	${BOOST_LIBS} )
set_property(TARGET MockUpCanImplementationcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET MockUpCanImplementationcan PROPERTY LINKER_LANGUAGE CXX)
