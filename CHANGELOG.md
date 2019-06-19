# Change Log
All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](http://keepachangelog.com/).

## todo
- create a decent pdf from the sphinx doc, via latex tools.
- protection against several createBus calls for systec/linux

## [1.1.5] 18.june.2019
### Fixed
- in 1.1.4, the anagate actually did not fully recover from a disconnect, the reception handler 
  was missing. Inserted a CANSetGlobals call to get the mod up again. The duo comes up
  relatively fast (20secs), and the X4/X8 take a bit longer to recuperate (60secs), but all
  reception handlers are fine after a power loss FOR CC7. For windows: not working...
  
  
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

