cmake_minimum_required(VERSION 2.8)

IF (WIN32)	
	add_subdirectory(systec)
	add_subdirectory(pkcan)
	add_subdirectory(anagate)
ELSE()	
	#add_subdirectory(sockcan)
	add_subdirectory(anagate)
ENDIF()

add_library (CanInterfaceImplementations OBJECT)
set_target_properties(CanInterfaceImplementations PROPERTIES LINKER_LANGUAGE Cxx)
