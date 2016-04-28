cmake_minimum_required(VERSION 2.8)

IF (WIN32)	
	add_subdirectory(systec)
	add_subdirectory(pkcan)
ELSE()	
	add_subdirectory(sockcan)
ENDIF()

add_library (CanInterfaceImplementations OBJECT)
set_target_properties(CanInterfaceImplementations PROPERTIES LINKER_LANGUAGE Cxx)