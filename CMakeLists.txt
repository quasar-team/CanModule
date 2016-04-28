cmake_minimum_required(VERSION 2.8)

add_subdirectory(CanInterfaceImplementations) #just to build the shared lib (reqd at runtime - loaded dynamically)
add_subdirectory(CanLibLoader)
add_subdirectory(CanInterface)


add_library (CommonCanComponent OBJECT)
set_target_properties(CommonCanComponent PROPERTIES LINKER_LANGUAGE Cxx)