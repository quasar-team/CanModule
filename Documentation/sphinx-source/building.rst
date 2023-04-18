========
Building
========

We use `cmake`_ 3.0.24 or higher for multi-OS building.
The dependencies are:

* `xerces`_-c 
* `boost`_ (1.81.0 preferred, but any version >=1.59.0 should work)
* `socketcan`_ libs (CC7, CAL9) (not to be confused with the CanModule sockcan.so lib!). 
  They can also be found in various places for open software.
* dependencies for CanModule: LogIt (sources to github pulled in by cmake)
* vendor libs (to be installed on your build/target machine)
* if general vendor builds are off you will build ONLY mockup. Using the toolchain:
   * SET( CANMODULE_BUILD_VENDORS "OFF") or UNSET( CANMODULE_BUILD_VENDORS ) or <nothing>
   * default is therefore "OFF" if you do nothing
* if you need the general vendor builds, and usually that is the case, then you need to switch them on explicitly in the toolchain:
   * SET( CANMODULE_BUILD_VENDORS "ON")
* you can switch off selected vendor builds which you do not need using the toolchain:
   * SET(CANMODULE_BUILD_SYSTEC "OFF")
   * SET(CANMODULE_BUILD_ANAGATE "OFF")
   * SET(CANMODULE_BUILD_PEAK "OFF")
   * by default, all selected vendors are built


**These dependencies should conveniently be injected into cmake using a toolchain file:**

.. code-block:: 

   CMakeLists.txt:
   ---------------
   (...other stuff)
   #
   # Load build toolchain file and internal checks before we start building
   #
   if( DEFINED CMAKE_TOOLCHAIN_FILE )
      message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CMAKE_TOOLCHAIN_FILE is defined -- including [${CMAKE_TOOLCHAIN_FILE}]")
      include( ${CMAKE_TOOLCHAIN_FILE} )    
   endif()
   (...other stuff)


**example toolchain for CAL9:**

.. code-block::

# toolchain for cal9 CANX
# cmake -DCMAKE_TOOLCHAIN_FILE= <toolchainname>.cmake .
#
# the toolchain just sets variables, but does not actually DO anything
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: ---toolchain start--- ")
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: CAL9 build for CANX" )
#SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
#
# boost
# compile e.g.: ./b2 link=static,shared threading=multi -j7 -- cxxflags="-fPIC" 
#
SET ( BOOST_PATH_LIBS "/opt/3rdPartySoftware/boost/boost_1_81_0/stage/lib" )
SET ( BOOST_PATH_HEADERS   "/opt/3rdPartySoftware/boost/boost_1_81_0" )
IF ($ENV{CANMODULE_AS_STATIC_AS_POSSIBLE} )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: as static as possible" )
	SET ( BOOST_LIBS
	libboost_log.a 
	libboost_log_setup.a 
	libboost_system.a 
	libboost_chrono.a 
	libboost_thread.a 
	libboost_date_time.a 
	libboost_filesystem.a 
	libboost_program_options.a 
	)
ELSE()
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: shared libs" )
	SET ( BOOST_LIBS
	-lboost_log 
	-lboost_log_setup 
	-lboost_system
	-lboost_chrono 
	-lboost_thread
	-lboost_date_time 
	-lboost_filesystem 
	-lboost_program_options 
  )
ENDIF()

# 
# LogIt, used by CANX directly as well
#
SET ( LOGIT_BACKEND_STOUTLOG ON CACHE BOOL "The basic backend to stdout" )
SET ( LOGIT_BACKEND_BOOSTLOG ON CACHE BOOL "Rotating file log with boost" )
SET ( LOGIT_BACKEND_UATRACELOG OFF CACHE BOOL "Unified Automation tookit logger" )
SET ( LOGIT_BACKEND_WINDOWS_DEBUGGER OFF CACHE BOOL "Windows debugger logger" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt Backend LOGIT_BACKEND_STOUTLOG= ${LOGIT_BACKEND_STOUTLOG} ]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt Backend LOGIT_BACKEND_BOOSTLOG= ${LOGIT_BACKEND_BOOSTLOG} ]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt Backend LOGIT_BACKEND_UATRACELOG= ${LOGIT_BACKEND_UATRACELOG} ]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: LogIt Backend LOGIT_BACKEND_WINDOWS_DEBUGGER= ${LOGIT_BACKEND_WINDOWS_DEBUGGER} ]" )

# allow rebuild on an already cloned LogIt
IF ( EXISTS ${PROJECT_BINARY_DIR}/build/LogIt )
	include_directories( build/build/LogIt/include )
ENDIF()
IF ( EXISTS ${PROJECT_BINARY_DIR}/build/lib )
	link_directories( build/build/lib )
ENDIF()

#
# xerces-c
#
SET ( XERCES_LIBS "-lxerces-c" )

# CanModule build behaviour:
# CANMODULE_BUILD_VENDORS OFF or not specified: only build mockup, do not build any vendor libs (default phony)
# CANMODULE_BUILD_VENDORS ON, nothing else: build mockup and all vendor libs (default all on)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_SYSTEC OFF: build mockup and all vendor libs except systec (drop systec)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_ANAGATE OFF: build mockup and all vendor libs except anagate (drop anagate)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_PEAK OFF: build mockup and all vendor libs except peak (drop peak)
SET(CANMODULE_BUILD_VENDORS ON )
# disable a vendor
#SET(CANMODULE_BUILD_SYSTEC OFF)
#SET(CANMODULE_BUILD_ANAGATE OFF)
#SET(CANMODULE_BUILD_PEAK OFF)
#
# we build CanModule from the sources, and we need headers and libs from the vendors
#
# linux: we use socketcan for systec and peak, and we always use the static lib, it is not big anyway
#
SET( SOCKETCAN_HEADERS  "/opt/3rdPartySoftware/CAN/CAN_libsocketcan/include" )
SET( SOCKETCAN_LIB_PATH "/opt/3rdPartySoftware/CAN/CAN_libsocketcan/src/.libs" )
SET( SOCKETCAN_LIB_FILE "libsocketcan.a" )
#
SET( SYSTEC_HEADERS ${SOCKETCAN_HEADERS} )
SET( SYSTEC_LIB_PATH ${SOCKETCAN_LIB_PATH} )
SET( SYSTEC_LIB_FILE ${SOCKETCAN_LIB_FILE} )
#
SET( PEAK_HEADERS ${SOCKETCAN_HEADERS} )
SET( PEAK_LIB_PATH ${SOCKETCAN_LIB_PATH} )
SET( PEAK_LIB_FILE ${SOCKETCAN_LIB_FILE} )
#
# beta v6 from dec 2022
SET ( ANAGATE_LIB_PATH "/opt/3rdPartySoftware/CAN/cal9/anagate/beta.v6/lib" )
SET ( ANAGATE_HEADERS  "/opt/3rdPartySoftware/CAN/cal9/anagate/beta.v6/include" )	
SET ( ANAGATE_LIB_FILE -lAnaGateExtStaticRelease -lAnaGateStaticRelease -lCANDLLStaticRelease64 )

# these versions link as well, shared
#SET ( ANAGATE_LIB_PATH "/opt/3rdPartySoftware/CAN/cal9/anagate/2.06-25.march.2021/lib" )
#SET ( ANAGATE_HEADERS  "/opt/3rdPartySoftware/CAN/cal9/anagate/2.06-25.march.2021/include" )
#SET ( ANAGATE_LIB_FILE "-lAPIRelease64 -lCANDLLRelease64" )


#
# special functions not using CanModule
#
SET ( SPECIAL_PATH_LIBS ${ANAGATE_PATH_LIBS} ) 
SET ( SPECIAL_HEADERS  ${ANAGATE_HEADERS} ) 
SET ( SPECIAL_LIB_FILES ${ANAGATE_LIB_FILE} -lAnaGateExtRelease -lAnaGateRelease )
message(STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}]: ---toolchain end--- ")



**example toolchain for W2022s:**

.. code-block:: 

# toolchain for w2022s on pcen33118 
# mludwig at cern dot ch
option(64BIT "64 bit build" ON) 

#
# boost
#	
# 1.81.0 seems to have a problem:
# First chance exception on 00007FFD1040EF5C (E06D7363, CPP_EH_EXCEPTION)!
#EXCEPTION_DEBUG_INFO:
#           dwFirstChance: 1
#           ExceptionCode: E06D7363 (CPP_EH_EXCEPTION)
#          ExceptionFlags: 00000081
#        ExceptionAddress: kernelbase.00007FFD1040EF5C
#        NumberParameters: 4
#ExceptionInformation[00]: 0000000019930520
#ExceptionInformation[01]: 0000000000000000
#ExceptionInformation[02]: 0000000000000000
#ExceptionInformation[03]: 0000000000000000
#First chance exception on 00007FFD1040EF5C (E06D7363, CPP_EH_EXCEPTION)!
SET ( BOOST_PATH_LIBS "C:/3rdPartySoftware/boost/boost_1_81_0/stage/lib" )
SET ( BOOST_PATH_HEADERS "C:/3rdPartySoftware/boost/boost_1_81_0" )
SET ( BOOST_LIBS 
	-lboost_log 
	-lboost_log_setup 
	-lboost_filesystem 
	-lboost_system
	-lboost_chrono 
	-lboost_date_time 
	-lboost_thread  )
	
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] toolchain defines [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] toolchain defines [BOOST_PATH_HEADERS:${BOOST_PATH_HEADERS}]" )
message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] toolchain defines [BOOST_LIBS:${BOOST_LIBS}]" )


#
# xerces-c
#
SET ( XERCES_PATH_LIBS "C:/3rdPartySoftware/xerces-c-3.2.4/src/Debug" )
SET ( XERCES_HEADERS "C:/3rdPartySoftware/xerces-c-3.2.4/src" )
SET ( XERCES_LIBS "xerces-c_3D.lib" )

# CanModule build behaviour:
# CANMODULE_BUILD_VENDORS OFF or not specified: only build mockup, do not build any vendor libs (default phony)
# CANMODULE_BUILD_VENDORS ON, nothing else: build mockup and all vendor libs (default all on)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_SYSTEC OFF: build mockup and all vendor libs except systec (drop systec)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_ANAGATE OFF: build mockup and all vendor libs except anagate (drop anagate)
# CANMODULE_BUILD_VENDORS ON, CANMODULE_BUILD_PEAK OFF: build mockup and all vendor libs except peak (drop peak)
SET(CANMODULE_BUILD_VENDORS "ON" )
# disable a vendor
#SET(CANMODULE_BUILD_SYSTEC "OFF")
#SET(CANMODULE_BUILD_ANAGATE "OFF")
#SET(CANMODULE_BUILD_PEAK "OFF")
#
# systec
# 6.june.2006
SET( SYSTEC_LIB_FILE "USBCAN64.lib")
SET( SYSTEC_HEADERS  "C:/3rdPartySoftware/CAN/windows/systec/6.june.2006/include")
SET( SYSTEC_LIB_PATH "C:/3rdPartySoftware/CAN/windows/systec/6.june.2006/lib" )

# anagate
# 2.06-25.march.2021
SET( ANAGATE_LIB_FILE "AnaGateCanDll64.lib")
SET( ANAGATE_HEADERS "C:/3rdPartySoftware/CAN/windows/anagate/2.06-25.march.2021/include" )
SET( ANAGATE_LIB_PATH "C:/3rdPartySoftware/CAN/windows/anagate/2.06-25.march.2021/lib" )
#
# beta.v6, hacked header AnaGateDll.h to force #define ANAGATEDLL_API __declspec(dllexport)
#SET( ANAGATE_LIB_FILE "AnaGateCanDll64.lib")
#SET( ANAGATE_HEADERS "C:/3rdPartySoftware/CAN/windows/anagate/beta.v6/include" )
#SET( ANAGATE_LIB_PATH "C:/3rdPartySoftware/CAN/windows/anagate/beta.v6/lib" )

# peak
# version PCAN Basic 4.3.2
SET( PEAK_LIB_FILE "PCANBasic.lib")
SET( PEAK_HEADERS  "C:/3rdPartySoftware/CAN/windows/peak/16.july.2022/Include" )
SET( PEAK_LIB_PATH "C:/3rdPartySoftware/CAN/windows/peak/16.july.2022/x64/VC_LIB" )


**The toolchain gets then injected by running cmake:**

.. code-block:: 

   cmake -DCMAKE_TOOLCHAIN_FILE=whateverToolchain.cmake
   
   
QA and documentation
====================

Local gitlab runners are used for the ics-fd-qa and documentation integration into the CI/CD. Therefore the `githubCanModule`_
repo is mirrored into `gitlabCanModule`_ and the .gitlab-ci.yml is executed on gitlab for the two stages only: 

- ics-fd-qa 
- documentation. 

.. _githubCanmodule: https://github.com/quasar-team/CanModule.git
.. _gitlabCanModule: https://gitlab.cern.ch/mludwig/canmodule-mirror.git
.. _jenkins:  https://ics-fd-cpp-master.web.cern.ch/view/CAN

The QA results are available at `sonarqube`_ under ics-fd-qa-CanModule-mirror

.. _sonarqube: https://cvl.sonarqube.cern.ch
