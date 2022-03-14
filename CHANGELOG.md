# Change Log
All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](http://keepachangelog.com/).

### in progress

- OPCUA-2614: extended messages, do we need an option ? 
- OPCUA-2584: rx/tx counting in the handler, for now, to improve err reporting already


### 2.0.17 [14.march.2022]
- OPCUA-2529: build chain version

### 2.0.16 [23.feb.2022]
- reconnection documentation updated and improved
- OPCUA-2604: reconnection behavior with firewall, sendMessage should not block. also fix the
    documentation about reconnection behavior.
    boost::mutex for anagate is replaced with std::mutex
    boost::signals are kept for anagate
- improved the message logging for anagate, is is now in hex characters and gives no unreadable chars any more (which mess up the terminal)

### 2.0.15 [22.feb.2022]
- OPCUA-2607: unblock RTR CAN messages, suppressed 3 lines of code for "sock", so that
  all vendors behave in the same way. Remote messages are not suppressed any more at reception.

### 2.0.14 [28.oct.2021]
- OPCUA-2452: standard CAN ID have 11bits, the API permits sending int16_t as IDs, with sign. Add a check that 0<ID<2048 and issue a WRN.
  Extended CAN messages are not supported, we stay with 8 bytes data.
- CANT-44: add a timeout parameter for anagate as 7th parameter


### 2.0.13 [ 1.july.2021 ]
- use std::mutex and scoped lock for sock instead of boost. Does not change much anyway. see
  OPCUA-2331 https://its.cern.ch/jira/browse/OPCUA-2331
- adding anagate timeout as an optional parameter to the list (in progress)

### 2.0.12 [15.june.2021]
- take out include <boost/thread/thread.hpp> dependency for windows

### 2.0.11 [4.JUNE.2021]
- fixing OPPCUA-2335: suppress the destructor call in CanLibLoader::closeCanBus(..)
- fix return type from constructor in Mock lib, so that the object gets created
- key-safeguards in Diag::delete_maps
- concept stopped bus:
  - cca->stopBus()
  - public method of the bus gets called by lib->closeCanBus()
  - can call cca->stopBus() as well directly
  - calls vendor specific methods to close the CAN bus
  - erases reception handler
  - deletes bus from bus map 
  - sets flag to prevent send
  - does not call object dtors, does not stop thread  
- concept closed bus:
  - lib->closeCanBus()
  - lib maps entry is deleted
  - calls stopBus()
- dtors for objects are called when 
  - object goes out of scope
  - explicit "delete cca" call on the (bus-) object




### 2.0.10
- return cca == NULL if first init is unsuccessful.
  client needs to check pointer, according to https://its.cern.ch/jira/browse/OPCUA-2248
- once a port is running, it reconnects as usual (no change)


### 2.0.9
- took out some remaining debugging lines
- corrected version to 2.0.9 (showed 2.0.7, was forgotten)
- sock@cc7: remove buses from bus map if config board has failed at init. Then retry until
  bus creating & config succeeds (forever). The intended behavior is
  that we loop around until the bus becomes available. In a multithreaded OPCUA server all the successful
  buses can be used in the meantime. When the broken busses become available the threads get unblocked.
- replaced boost sleep with C++ sleep everywhere to reduce dependencies

### 2.0.8
- adding CanModule (filimonov) wrapper again to solve https://its.cern.ch/jira/browse/OPCUA-2098
  CanInterface/CanBusAccess.cpp/h added
- testing wrapper accordingly


## 2.0.7
- OPCUA-1691: replaced pthreads with std::thread C++11. Call non-static private member method on embedding object,
  as this is the best option to profit from inheritance (which is deeply rooted into CanModule, unfortunately)
- doc rerun up to date 

## 2.0.6
- with CANX 2.0.6
- documentation updated
- lost of little fixes and bug corrections
- proper vendor-related error signals now, for all implementations


## 2.0.4
- added an infinite retry if init open port fails. works very well for anagate
- took out some debugging leftovers
- use library file name ( .so or .dll) as firts part in key: i.e. ancan.dll_0::an:0_0
  means "anagate windows lib instanc0, port0, instance0". The following _X is always the 
  instance count.
- added method getPortBitrate(), and updated standardApi documentation
- cleaned up documentation, except the reconnection chapter
- tests and verifications are still going on


## 2.0.0rc
- https://its.cern.ch/jira/browse/OPCUA-2014
  according to vendor and OS, acquire bus status, and return one uint32_t bitpattern which has
  the same rules for all vendors. In fact the status for vendors is too different to be abstracted
  into a common bitpattern.
- https://its.cern.ch/jira/browse/OPCUA-1998
  LD_LIBRARY_PATH replaced by rpath in the cmake toolchain. Hope it is OK now.
- https://its.cern.ch/jira/browse/OPCUA-1996
  vendor libs are at https://gitlab.cern.ch/mludwig/canmodulevendorlibs pls README.md
- https://its.cern.ch/jira/browse/OPCUA-1913
  dead code supressed
- https://its.cern.ch/jira/browse/OPCUA-1905
  deeper issue fixed, can have more than 4 buses for anagate
- https://its.cern.ch/jira/browse/OPCUA-1902
  doc revamped, should be better, still in progress as always
- https://its.cern.ch/jira/browse/OPCUA-1735
  PEAK PCAN: fix can port number to certain device
  solved, making calls to the udev system, using device identifier in each peak module
  and specifying this as "extended device ID" to the CanModule. 
- https://its.cern.ch/jira/browse/OPCUA-1581
  anagate high speed flag gets set automatically, but API is bw compatible
- https://its.cern.ch/jira/browse/ENS-26903
  anagate default termination is ON now (was off)
- https://its.cern.ch/jira/browse/JCOP-763
  shared modules and reconnection behavior done in v2.0
- https://its.cern.ch/jira/browse/CANT-40
  statistics related: canx shows this correctly. 

remaining open for the moment (no time yet, somewhat lower prio)
- https://its.cern.ch/jira/browse/OPCUA-1691
- https://its.cern.ch/jira/browse/OPCUA-794
- https://its.cern.ch/jira/browse/OPCUA-629
- https://its.cern.ch/jira/browse/CANT-38 
  have to print the chars as hex to avoid the nonprinting chars at reception (sockcan)
- https://its.cern.ch/jira/browse/OPCUA-2027

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

