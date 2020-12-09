=========   
CAN ports
=========
   
port bit rate and statistics
============================

.. doxygenclass:: CanModule::CCanAccess 
   :project: CanModule
   :members: getPortBitrate, getStatistics
   
port unified status
===================
 a 32-bit pattern, with
 
 - 0xF0.00.00.00 = code for implementation
 - 0x0F.FF.FF.FF = native status from the vendor API (not all bits used) 

.. doxygenclass:: CanModule::CCanAccess 
   :project: CanModule
   :members: getPortStatus
   
syntax for port names
=====================

CAN ports are specified by the user with an integer number 0..N.
Vendors and implementations tend to handle that differently, but CanModule tries to provide a
standard API across all vendors. The following strings, specified for CAN ports, will connect 
to can port P:

* "P"
* "canP"
* "moduleP"
* "whateverP"

but specifying

* "vcanP"
* "vcanmoduleP"

will use vcan (virtual can) instead under linux for USB/socketcan bridges. P has to be an integer. 
See vendor specific sections for the parameters and port identification as well, there are differences between
the implementations. 

* The **access to a CAN port** is through:

.. doxygenclass:: CanModule::CCanAccess
   :project: CanModule
   :members: openCanBus, closeCanBus



.. _CANX: https://gitlab.cern.ch/mludwig/canx.git

