cmake_minimum_required(VERSION 2.8)
project(ancan)

SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/CanInterfaceImplementations/output/)

IF(WIN32)
	SET(ANAGATE_INCLUDE_DIR "C:/3rdPartySoftware/AnaGateCAN/win64/vc8/include")
	SET(ANAGATE_LIB "C:/3rdPartySoftware/AnaGateCAN/win64/vc8/Release/AnaGateCanDll64.lib")
ELSE()
	SET(ANAGATE_INCLUDE_DIR "/opt/CAN/anagate-api-2.13/linux64/gcc4_6/include")
	SET(ANAGATE_LINK_DIR /opt/CAN/anagate-api-2.13/linux64/gcc4_6 /opt/CAN/libAnaGate-1.0.9/linux64/gcc4_6 /opt/CAN/libAnaGateExt-1.0.3/linux64/gcc4_6)
	SET(ANAGATE_LIBRARY_NAME CANDLLRelease64 APIRelease64 AnaGateRelease AnaGateExtRelease)
ENDIF()

set(ANAGATE_SOURCES
	AnaCanScan.cpp
	AnaCanScan.h
	${CMAKE_SOURCE_DIR}/CanInterface/src/CanStatistics.cpp
	${CMAKE_SOURCE_DIR}/CanInterface/include/CanStatistics.h
	${CMAKE_SOURCE_DIR}/CanInterface/src/CanModuleUtils.cpp
	${CMAKE_SOURCE_DIR}/CanInterface/include/CanModuleUtils.h
)

include_directories ( ${ANAGATE_INCLUDE_DIR} )

if("${LOGIT_BUILD_OPTION}" STREQUAL "LOGIT_AS_INT_SRC")
	add_library( ancan SHARED ${ANAGATE_SOURCES} $<TARGET_OBJECTS:LogIt>)
else()
	add_library( ancan SHARED ${ANAGATE_SOURCES} )
	target_link_libraries( ancan ${LOGIT_LIB} )
endif()

target_link_libraries ( ancan ${BOOST_LIBS}	${ANAGATE_LIB} )
set_property(TARGET ancan PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET stcan PROPERTY LINKER_LANGUAGE CXX)