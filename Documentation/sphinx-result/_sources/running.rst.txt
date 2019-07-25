=======
Running
=======

The proper kernel modules, drivers and libraries for the vendors used through CanModule 
have to be present during runtime. Please also refer to `Status`_ for an overview over 
runtime conditions (april 2019).


general dependencies
--------------------
* CanModule.dll/.so
* LogIt (cloned from github during cmake)
* boost 1.64.0
* xerces 3.2 (xerces-c_3_2D.dll)

Anagate
-------
* libancan.dll/.so  (standard API)
* linux: libAPIRelease64.so, libCANDLLRelease64.so, libAnaGateExtRelease.so, libAnaGateRelease.so
* windows: ANAGATECAN64.dll

Systec
------
* linux: libsockcan.so (standard API), driver kernel module systec_can.ko and dependent modules
* windows: libstcan.dll (standard API), USBCAN64.dll

Peak
----
* linux: libsockcan.so (standard API), driver kernel module pcan.ko and dependent modules
* windows: libpkcan.dll (standard API), PKCANBASIC.dll


shared - reduced dependencies (*-static)
----------------------------------------

During execution, CanModule (*-static.so or *.so) looks for the standard shared libs.
If you want to use the "reduced dependencies" versions then the env var CANMODULE_AS_STATIC_AS_POSSIBLE=1
has to be set **during runtime** as well.

See also "building"

.. _Status: https://edms.cern.ch/file/2089743/1/CanModuleStatus2019_v4.pptx
  
  