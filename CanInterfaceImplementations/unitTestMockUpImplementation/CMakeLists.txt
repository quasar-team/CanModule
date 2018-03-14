cmake_minimum_required(VERSION 2.8)

IF(WIN32)
	SET(MOCKUP_GETTIMEOFDAY ../../CanInterface/src/gettimeofday.cpp)
ENDIF()

message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: STANDALONE_BUILD [${STANDALONE_BUILD}] STANDALONE_BUILD_SHARED [${STANDALONE_BUILD_SHARED}]")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)

include_directories (	
${PROJECT_SOURCE_DIR}/CanModule/CanInterface/include
${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/unitTestMockUpImplementation
)

set(MOCK_SOURCES
	MockCanAccess.cpp
	MockCanAccess.h
	../../CanInterface/src/CanStatistics.cpp
	../../CanInterface/src/CanModuleUtils.cpp
	${MOCKUP_GETTIMEOFDAY}
)

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( MockUpCanImplementation SHARED ${MOCK_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( MockUpCanImplementation SHARED ${MOCK_SOURCES} )
	target_link_libraries( CanMoMockUpCanImplementationdule ${LOGIT_LIB} )
endif()
target_link_libraries ( MockUpCanImplementation	${BOOST_LIBS} )

set_property(TARGET MockUpCanImplementation PROPERTY POSITION_INDEPENDENT_CODE TRUE)
