cmake_minimum_required(VERSION 2.8)

IF(WIN32)
	SET(ANAGATE_INCLUDE_DIR "C:/AnagateCAN/win64/vc8/include")
	SET(ANAGATE_LINK_DIR "C:/AnagateCAN/win64/vc8/Release")
	SET(ANAGATE_LIBRARY_NAME AnagateCanDll64)
	SET(ANAGATE_GETTIMEOFDAY ../../CanInterface/src/gettimeofday.cpp)
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
ELSE()
	SET(ANAGATE_INCLUDE_DIR /opt/CAN/anagate-api-2.11/linux64/gcc4_6/include)
	SET(ANAGATE_LINK_DIR /opt/CAN/anagate-api-2.11/linux64/gcc4_6 /opt/CAN/libAnaGate-1.0.9/linux64/gcc4_6 /opt/CAN/libAnaGateExt-1.0.3/linux64/gcc4_6)
	SET(ANAGATE_LIBRARY_NAME CANDLLRelease64 APIRelease64 AnaGateRelease AnaGateExtRelease)
	SET(ANAGATE_GETTIMEOFDAY )
	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/output/)
ENDIF()


include_directories (	
${PROJECT_SOURCE_DIR}/CanModule/CanInterface/include
${PROJECT_SOURCE_DIR}/CanModule/CanInterfaceImplementations/anagate
${ANAGATE_INCLUDE_DIR}
)
link_directories(
${ANAGATE_LINK_DIR}
)
add_library (AnagateCanImplementation SHARED
AnaCanScan.cpp
../../CanInterface/src/CanStatistics.cpp
../../CanInterface/src/CanModuleUtils.cpp
${ANAGATE_GETTIMEOFDAY}
$<TARGET_OBJECTS:LogIt>
)
#set_target_properties(AnagateCanImplementation PROPERTIES LINKER_LANGUAGE Cxx)
target_link_libraries ( AnagateCanImplementation 
	${BOOST_LIBS}

	${ANAGATE_LIBRARY_NAME}
)
