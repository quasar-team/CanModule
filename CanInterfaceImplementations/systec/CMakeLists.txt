cmake_minimum_required(VERSION 2.8)

add_definitions("/EHsc")
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterfaceImplementations/output/)
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterfaceImplementations/output/)
include_directories (	
${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterface/include
${PROJECT_SOURCE_DIR}/CommonCanComponent/CanInterfaceImplementations/systec
"C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Include"
)
link_directories(
"C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Lib/"
)
add_library (SystecCanImplementation SHARED
STCanScan.cpp
../../CanInterface/src/CanStatistics.cpp
../../CanInterface/src/CCCUtils.cpp
../../CanInterface/src/gettimeofday.cpp
$<TARGET_OBJECTS:LogIt>
)

target_link_libraries ( SystecCanImplementation 
	${BOOST_LIBS}

	USBCAN64
)