=======
Running
=======

The proper kernel modules, drivers and libraries for the vendors used through CanModule 
have to be present during runtime obviously. You will need the `CanModuleVendorLibs`_ .


general dependencies
--------------------
* CanModule.dll/.so
* LogIt (cloned from github during cmake)
* boost 1.75.0 or similar (depending on your build)
* xerces 3.2 (xerces-c_3_2D.dll)

Anagate
-------
* libancan.dll/.so  (standard API)
* linux: libAPIRelease64.so, libCANDLLRelease64.so, libAnaGateExtRelease.so, libAnaGateRelease.so
* windows: ANAGATECAN64.dll

Anagate2 (post july 2023)
--------
* liban2can.dll/.so
* linux: libAnaGateExtStaticRelease.so, libAnaGateStaticRelease.so, libCANDLLStaticRelease64.so
* windows: AnaGateCan64.dll, AnaGateCanDll64.dll 

Systec
------
* linux: libsockcan.so (standard API), driver kernel module systec_can.ko and dependent modules, libsocketcan.so
* windows: libstcan.dll (standard API), USBCAN64.dll

Peak
----
* linux: libsockcan.so (standard API), driver kernel module pcan.ko and dependent modules, libsocketcan.so
* windows: libpkcan.dll (standard API), PKCANBASIC.dll


.. _CanModuleVendorLibs: https://gitlab.cern.ch/mludwig/canmodulevendorlibs
  
  
