cmake_minimum_required(VERSION 2.8)
project(stcan)

#SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
#SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
include_directories (	
${PROJECT_SOURCE_DIR}/CanModule/CanInterface/include
${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/systec
"C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Include"
)
link_directories(
"C:/Program Files (x86)/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Lib/"
)
add_library (stcan SHARED
STCanScan.cpp
STCanScan.cpp
$<TARGET_OBJECTS:LogIt>
$<TARGET_OBJECTS:CanModule>
)
set_property(TARGET stcan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries ( stcan 
	${Boost_LIBRARIES}
	USBCAN32
)