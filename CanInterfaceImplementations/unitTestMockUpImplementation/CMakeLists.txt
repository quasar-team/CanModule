cmake_minimum_required(VERSION 2.8)

IF(WIN32)
	SET(MOCKUP_GETTIMEOFDAY ../../CanInterface/src/gettimeofday.cpp)
ENDIF()

message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: STANDALONE_BUILD [${STANDALONE_BUILD}] STANDALONE_BUILD_SHARED [${STANDALONE_BUILD_SHARED}]")
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using boost libraries: BOOST_LIBS [${BOOST_LIBS}]")  
message(STATUS "file [${CMAKE_CURRENT_LIST_FILE}]: Using LogIt library: LOGIT_LIB [${LOGIT_LIB}]")

include_directories (	
${PROJECT_SOURCE_DIR}/CanModule/CanInterface/include
${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/unitTestMockUpImplementation
)
add_library (MockUpCanImplementation SHARED
MockCanAccess.cpp
MockCanAccess.h
../../CanInterface/src/CanStatistics.cpp
../../CanInterface/src/CanModuleUtils.cpp
${MOCKUP_GETTIMEOFDAY}
)

target_link_libraries ( MockUpCanImplementation	${BOOST_LIBS} ${LOGIT_LIB})
