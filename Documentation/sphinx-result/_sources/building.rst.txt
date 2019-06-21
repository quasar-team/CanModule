========
Building
========
.. get supported languages for code-blocks with: pygmentize -L lexers
.. this shows a huuuge amount of languages available, I choose basemake 
.. for cmake stuff

We use `cmake`_ 2.8 or higher for multi-OS building.
The dependencies are:

* xerces-c
* boost (1.64.0 preferred, but any version >=1.59.0 should work)
* socketcan libs (CC7) (not to be confused with the CanModule sockcan.so lib!)
* dependencies for CanModule: LogIt (sources gto github pulled in by cmake)
* vendor libs (to be installed on your build/target machine)
* you will have to switch off vendor builds if you want ONLY mockup, using the toolchain
	* SET( CANMODULE_BUILD_VENDORS "OFF")
	* default is "ON"
* you can switch off selected vendor which you do not need using the toolchain
	* SET(CANMODULE_BUILD_SYSTEC "OFF")
	* SET(CANMODULE_BUILD_ANAGATE "OFF")
	* SET(CANMODULE_BUILD_PEAK "OFF")


**These dependencies should conveniently be injected into cmake using a toolchain file:**

.. code-block:: basemake

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

**The toolchain file would look like this (i.e. windows):**

.. code-block:: basemake

	toolchain.cmake:
	----------------
	# toolchain for CANX-tester for CI jenkins, w10e
	# mludwig at cern dot ch
	# cmake -DCMAKE_TOOLCHAIN_FILE=jenkins_CanModule_w10e.cmake .
	#
	# boost
	#	
	# bin download from sl	
	SET ( BOOST_PATH_LIBS "M:/3rdPartySoftware/boost_1_59_0-msvc-14/lib64" )
	SET ( BOOST_HEADERS "M:/3rdPartySoftware/boost_1_59_0-msvc-14" )
	SET ( BOOST_LIBS -lboost_log -lboost_log_setup -lboost_filesystem -lboost_system -lboost_chrono -lboost_date_time -lboost_thread  )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] toolchain defines [BOOST_PATH_LIBS:${BOOST_PATH_LIBS}]" )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] toolchain defines [BOOST_HEADERS:${BOOST_HEADERS}]" )
	message( STATUS "[${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}] toolchain defines [BOOST_LIBS:${BOOST_LIBS}]" )
	# 
	# LogIt, used by CANX directly as well
	#
	SET ( LOGIT_HEADERS   "$ENV{JENKINSWS}/CanModule/LogIt/include" )
	SET ( LOGIT_PATH_LIBS "$ENV{JENKINSWS}/CanModule/LogIt/lib" )
	SET ( LOGIT_LIBS "-lLogIt" )
	#
	# xerces-c
	#
	SET ( XERCES_PATH_LIBS "M:/3rdPartySoftware/xerces-c-3.2.0_64bit/src/Debug" )
	SET ( XERCES_HEADERS "M:/3rdPartySoftware/xerces-c-3.2.0_64bit/src" )
	SET ( XERCES_LIBS "xerces-c_3D.lib" )
	#
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
	# version 6.02 for windows 10 7may2018
	SET( SYSTEC_LIB_FILE "M:/3rdPartySoftware/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Lib/USBCAN64.lib")
	SET( SYSTEC_HEADERS "M:/3rdPartySoftware/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/Include")
	SET( SYSTEC_LIB_PATH "M:/3rdPartySoftware/SYSTEC-electronic/USB-CANmodul Utility Disk/Examples/lib" )
	#
	# anagate
	# version vc8 as it seems
	SET( ANAGATE_LIB_FILE "AnaGateCanDll64.lib")
	SET( ANAGATE_HEADERS "M:/3rdPartySoftware/AnaGateCAN/win64/vc8/include" )
	SET( ANAGATE_LIB_PATH "M:/3rdPartySoftware/AnaGateCAN/win64/vc8/Release" )
	#
	# peak
	# version PCAN Basic 4.3.2
	SET( PEAK_LIB_FILE "PCANBasic.lib")
	SET( PEAK_HEADERS "M:/3rdPartySoftware/PCAN-Basic API/Include" )
	SET( PEAK_LIB_PATH "M:/3rdPartySoftware/PCAN-Basic API/x64/VC_LIB" )

**The toolchain gets then injected by running cmake:**

.. code-block:: basemake

	cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
	
	
.. _cmake: https://cmake.org/
