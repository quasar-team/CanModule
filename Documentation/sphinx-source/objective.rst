==========
Objective
==========

**CanModule** is a software abstraction layer, written in C++, to simplify integration
of CAN bridges from different vendors into cmake (C++) projects needing CAN connectivity
for windows and linux. A CAN bridge is - usually - an external module which is connected
on one side to a computer or network and offers CAN ports on the other side where CAN buses
can be connected.

The original authors are the CERN Atlas-DCS team, support is now done by BE-ICS-FD.


Supported OS
------------

These operating systems are supported for all vendors

* CERN CC7 
* Windows 2016 Server and Windows 10

other OS versions might be available on special request, or easily be ported to. 
 
compatible vendors
------------------

CAN bridges from vendors are compatible with CanModule and are tested
 
* `AnaGate`_
* `SysTec`_ 
* `Peak`_ 

Integration into projects
------------------------

In order to use CanModule in your  project/server/code there are two ways 

* by source-clone from `CanModuleGitlab`_ and integrated into a cmake build chain.
* binaries to copy for other build chains are available CERN-wide from `NexusCan`_ .  





.. _AnaGate: http://www.anagate.de/en/products/can-ethernet-gateways.htm
.. _SysTec: https://www.systec-electronic.com/
.. _Peak: https://www.peak-system.com/

.. _CanModuleGitlab: https://github.com/quasar-team/CanModule.git
.. _NexusCan: https://repository.cern.ch/nexus/content/repositories/cern-can/


