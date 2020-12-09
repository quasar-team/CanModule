========
Building
========

We use `cmake`_ 3.0 or higher for multi-OS building.
The dependencies are:

* `xerces`_-c 
* `boost`_ (1.64.0 or later preferred, but any version >=1.59.0 should work)
* `socketcan`_ libs (CC7, ubuntu) (not to be confused with the CanModule sockcan.so lib!). 
  They can also be found in various places for open software.
* dependencies for CanModule: LogIt (sources to github pulled in by cmake)
* vendor libs (to be installed on your build/target machine)
* if general vendor builds are off you will build ONLY mockup. Using the toolchain:
   * SET( CANMODULE_BUILD_VENDORS "OFF") or UNSET( CANMODULE_BUILD_VENDORS ) or <nothing>
   * default is therefore "OFF" if you do nothing
* if you need the general vendor builds, and usually that is the case, then you need to 
  switch them on explicitly in the toolchain:
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

**The toolchain file would look like this (i.e. windows):**

.. code-block:: 

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
   SET(CANMODULE_BUILD_PEAK "OFF")
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
   ## peak
   ## version PCAN Basic 4.3.2
   ##SET( PEAK_LIB_FILE "PCANBasic.lib")
   ##SET( PEAK_HEADERS "M:/3rdPartySoftware/PCAN-Basic API/Include" )
   ##SET( PEAK_LIB_PATH "M:/3rdPartySoftware/PCAN-Basic API/x64/VC_LIB" )

**The toolchain gets then injected by running cmake:**

.. code-block:: 

   cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
   
   
shared and static linking
-------------------------

CanModule uses all shared libraries, and also loads shared libraries during runtime for each connection
object and vendor. Nevertheless some shared libraries with reduced dependencies can be built in
some cases for linux: they have suffix *-static.so .
If the vendor APIs come in the form of relocateable static libraries/archives (-fPIC), then they
can sometimes be integrated as static into the CanModule specific vendor shared lib.

**Linux (CC7, ubuntu):**
if the environment variable CANMODULE_AS_STATIC_AS_POSSIBLE=1 is set during building the *-static 
libs are produced with boost and other specific dependencies integrated as possible:

* ancan-static.so
* sockcan-static.so
* CanModule-static.so

During execution, CanModule(-static.so or .so) looks for the standard shared libs, but sockewtcan and boost
are linked into static. 
If you want to use the "reduced dependencies" versions then the env var CANMODULE_AS_STATIC_AS_POSSIBLE=1
has to be set **during runtime** as well.

* boost_1_74_0 (or such) has to be built to provide both shared and static libs:
cd ./boost/boost_1_7XX_0
./bootstrap.sh 
./b2 -j 7 link=static,shared threading=multi define=BOOST_DISABLE_WIN32 cxxflags="-fPIC"


**Windows**

No reduced dependencies libs are available at this point.
   
.. _cmake: https://cmake.org/
.. _xerces: http://xerces.apache.org/xerces-c/
.. _boost: https://www.boost.org/
.. _socketcan: https://gitlab.cern.ch/mludwig/CAN_libsocketcan


QA and documentation
====================

Local gitlab runners are used for the ics-fd-qa and documentation integration into the CI/CD. Therefore the `githubCanModule`_
repo is mirrored into `gitlabCanModule`_ and the .gitlab-ci.yml is executed on gitlab for the two stages only: 

- ics-fd-qa 
- documentation. 

No libraries or binaries are built, since anyway CanModule is cross-platform and cannot be built entirely on gitlab 
runners. A jenkins instance at `jenkins`_ is used instead.

.. _githubCanmodule: https://github.com/quasar-team/CanModule.git
.. _gitlabCanModule: https://gitlab.cern.ch/mludwig/canmodule-mirror.git
.. _jenkins:  https://ics-fd-cpp-master.web.cern.ch/view/CAN

The QA results are available at `sonarqube`_ under ics-fd-qa-CanModule-mirror

.. _sonarqube: https://cvl.sonarqube.cern.ch




