cmake_minimum_required(VERSION 2.8)

add_definitions("/EHsc")
#SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
#SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
include_directories (	
${PROJECT_SOURCE_DIR}/CanModule/CanInterface/include
${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/pkcan
"C:/Works/PCAN-Basic API/Include"
)
link_directories(
"C:/Works/PCAN-Basic API/Win32/VC_LIB"
)
add_library (pkcan SHARED
pkcan.cpp
pkcan.h
$<TARGET_OBJECTS:LogIt>
$<TARGET_OBJECTS:CanModule>
)	

target_link_libraries ( pkcan 
	${Boost_LIBRARIES}

	PCANBasic
)