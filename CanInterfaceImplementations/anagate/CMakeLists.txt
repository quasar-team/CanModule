cmake_minimum_required(VERSION 2.8)

add_definitions("/EHsc")
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterfaceImplementations/output/)
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterfaceImplementations/output/)
include_directories (	
${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterface/include
${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterfaceImplementations/anagate
"C:/AnagateCAN/win64/vc8/include"
)
link_directories(
"C:/AnagateCAN/win64/vc8/Release"
)
add_library (AnagateCanImplementation SHARED
AnaCanScan.cpp
../../CanInterface/src/CanStatistics.cpp
../../CanInterface/src/CCCUtils.cpp
../../CanInterface/src/gettimeofday.cpp
$<TARGET_OBJECTS:LogIt>
)

target_link_libraries ( AnagateCanImplementation 
	${BOOST_LIBS}

	AnaGateCanDll64
)