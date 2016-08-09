cmake_minimum_required(VERSION 2.8)

IF(WIN32)
	SET(MOCKUP_GETTIMEOFDAY ../../CanInterface/src/gettimeofday.cpp)
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
ELSE()
	SET(MOCKUP_GETTIMEOFDAY )
	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
ENDIF()


include_directories (	
${PROJECT_SOURCE_DIR}/CanModule/CanInterface/include
${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/unitTestMockUpImplementation
)
add_library (MockUpCanImplementation SHARED
mockUp.cpp
../../CanInterface/src/CanStatistics.cpp
../../CanInterface/src/CanModuleUtils.cpp
${MOCKUP_GETTIMEOFDAY}
$<TARGET_OBJECTS:LogIt>
)
#set_target_properties(AnagateCanImplementation PROPERTIES LINKER_LANGUAGE Cxx)
target_link_libraries ( MockUpCanImplementation 
	${BOOST_LIBS}
)
