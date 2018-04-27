cmake_minimum_required(VERSION 2.8)

IF(WIN32)
	SET(ANAGATE_INCLUDE_DIR "C:/AnagateCAN/win32/vc8/include")
	SET(ANAGATE_LINK_DIR "C:/AnagateCAN/win32/vc8/Release")
	SET(ANAGATE_LIBRARY_NAME AnagateCanDll )
#	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
#	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
ELSE()
	SET(ANAGATE_INCLUDE_DIR /opt/CAN/anagate-api-2.13/linux64/gcc4_6/include)
	SET(ANAGATE_LINK_DIR /opt/CAN/anagate-api-2.13/linux64/gcc4_6 /opt/CAN/libAnaGate-1.0.9/linux64/gcc4_6 /opt/CAN/libAnaGateExt-1.0.3/linux64/gcc4_6)
	SET(ANAGATE_LIBRARY_NAME CANDLLRelease64 APIRelease64 AnaGateRelease AnaGateExtRelease)
    SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/)
#	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
ENDIF()


include_directories (	
${PROJECT_SOURCE_DIR}/CanModule/CanInterface/include
${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/anagate
${ANAGATE_INCLUDE_DIR}
)
link_directories(
${ANAGATE_LINK_DIR}
)
add_library (ancan SHARED
AnaCanScan.cpp
AnaCanScan.h
#../../CanInterface/src/CanStatistics.cpp
#../../CanInterface/src/CanModuleUtils.cpp
$<TARGET_OBJECTS:LogIt>
$<TARGET_OBJECTS:CanModule>
)
target_link_libraries ( ancan 
	${Boost_LIBRARIES}
	${ANAGATE_LIBRARY_NAME}
)
set_property(TARGET ancan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
