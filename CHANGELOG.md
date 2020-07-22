# Change Log
All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](http://keepachangelog.com/).

## 2.0.0.0
- OPCUA-1913: isCanPortOpen() suppressed as it is dead code, including the map m_ScanManagers
  The connection/reconnection behavior per port is under review anyway
- OPCUA-1581 (anagate gigh speed flag suppressed): merged branch
- OPCUA-1735 (peak connect to deterministic module): merged branch
- ENS26903: (default termination for anagate HIGH): merged branch

   
## 1.1.10.0
- OPCUA-1735 for peak bridges
- OPCUA-1735: that is a fat one, fixing peak non-deterministic port numbering.
  in the strict sense this is just a bug fix, but we'll have it as a new
  featuer because you can specify "sock:can0"device123" now. it is bw compatible as well
- OPCUA-1581: anagate highspeed flag set automatically, feature, but bw compatible 
- make a minor release increment because the behaviour IS NOT fully backward compatible in the strict sense. ELMB boards

need a termination (Alice)
- ENS-26903: the default behavior for anagate chanhes from no termination to termination (channel ohmic termination

## 1.1.9.6
- added toolchain for ubuntu, the obnly differences to cc7 come from the fact that a 
  physical jenkins slave for ubuntu has to be used right now. Nexus delivers into ubuntu1804LTS
  We can support ubuntu inofficially.
- separated build for dynamical and as-static-as-possible libs for CanModule, anagate and 
  socketcan (=peak, systec) for all linux flavours. Set env var "CANMODULE_AS_STATIC_AS_POSSIBLE=1"
  for the -static lib versions. Default is of course dynamical (env var not set).

## 1.1.9.5
- disconnect and reconnect on the same bus by software: connection map management corrected
- double-create of one bus protected
- double-close of one bus protected
- works for systec, peak and anagate @linux
- increased anagate open can bus timeout from 6 to 12 seconds
- force a delay of 7 sec. after anagate close bus to avoid fw crash too soon

## 1.1.9.4 [14.oct.2019]
- sync flag suppressed
- hi speed to true as default for anagate

## 1.1.9.3 [1.aug.2019]
- cleanup and pruned documentation


## [1.1.9.2] 1.aug.2019
- bugfix-OPCUA-1474: header file was wrong for systec@windows


## [1.1.9.0] 26.july.2019
* reduced dependency libs can be build and used, set environment 
        export CANMODULE_AS_STATIC_AS_POSSIBLE=1
   for build and also runtime
* essentially reduce dependency to boost and libsocketcan, for linux only
* for windows no changes
* default behaviour is "all shared" as before
* LogIt registering ONE component ONCE (Slava detected that ;-)
* dropped some INF Logging messages
* cmake 3.0 or higher, same as for quasar, to have much neater versioning
* versioning ONLY in the top-level CMakeLists.txt, this creates a file VERSION.h
* VERSION.h is included, and used to time-version-stamp the bins/libs
* can identify the version of a lib like that, and when it was built
* also output to ./lib for all artifacts, NOT to CanInterfaceImplementations/output any more

## [1.1.7] 21.june.2019
### Changed
- switch off unneeded vendors optionally
- updated doc

## [1.1.6] 20.june.2019
### Fixed
- build chain cleanup

## [1.1.5] 18.june.2019
### Fixed
- in 1.1.4, the anagate actually did not fully recover from a disconnect, the reception handler 
  was missing. Inserted a CANSetGlobals call to get the mod up again. The duo comes up
  relatively fast (20secs), and the X4/X8 take a bit longer to recuperate (60secs), but all
  reception handlers are fine after a power loss FOR CC7. For windows, the reply
  handlers are NOT reconnected, and the obj. map stays as before. The windows API 
  behaves differently, but that is also working now.
  reconnection tests: anagate duo , X4/X8, on w2016s, w10e, cc7, w2008r2 - all ok
     
  
### 1.1.3, 1.1.4 are intermediate, some bugs, don't use  

## [1.1.2] 23.may.2019
### Added
- PEAK works for USB / USB Pro briges for linux and windows, but NOT for PEAK-USB FD
  because the FD mods need some slightly different API calls. A few if-s are therefore
  needed extra for FD modules, and some more parameters to set up the FD CAN traffic. This can
  be passed through the parameters, and code is somewhat already prepared for this. It should not
  be a big problem to add PEAK-USB FD support therefore. The PCAN_Basic interface/driver
  used for windows is compatible with almost all PEAK hardware, also PCI and .m2 bridges.
  Linux uses socketcan, that seems to work also with FD modules (that is unofficial still). 
- component logging for each vendor
- decent developers documentation made with sphinx/breathe/doxygen

### Fixed
- OPCUA-1415, as a quickfix as proposed by Slava

### Changed
- tag is now 1.1.2

### [1.1.1]
## (component logging and sphinx)


## [1.0.1] 11.march.2019
### Added
- anagate irmware reboot, reconnection works as well now. So all 
  three ways to take down an anagate module (power, network, firmware) lead to a Canmodule reconnection
### Changed
### Fixed

## [1.0.0] - 7.feb.2019 - production release
### Added
- anagate and systec support for cc7 and windows
### Changed
### Fixed

