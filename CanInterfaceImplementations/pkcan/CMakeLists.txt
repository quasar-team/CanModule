cmake_minimum_required(VERSION 2.8)

add_definitions("/EHsc")
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterfaceImplementations/output/)
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterfaceImplementations/output/)
include_directories (	
${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterface/include
${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterfaceImplementations/pkcan
"C:/Works/PCAN-Basic API/Include"
)
link_directories(
"C:/Works/PCAN-Basic API/x64/VC_LIB"
)
add_library (PeakCanImplementation SHARED
pkcan.cpp
../../CanInterface/src/CanStatistics.cpp
../../CanInterface/src/CCCUtils.cpp
../../CanInterface/src/gettimeofday.cpp
$<TARGET_OBJECTS:LogIt>
)	

target_link_libraries ( PeakCanImplementation 
	${BOOST_LIBS}

	PCANBasic
)