=========
`SysTec`_
=========

All modules from vendor `SysTec`_ are handled by class STCanScan (windows) or CSockCanScan (linux) which 
manage the modules through their underlying vendor specific API and provide the standard generic CanModule API.
Here the underlying vendor specific classes and the specific parameters are documented. 

SysTec modules USB-CAN bridges are supported: sysWORXX 1,2,8,16

status
------
status information is propagated through the unified status.
For windows:

.. doxygenclass:: STCanScan 
   :project: CanModule
   :members: getPortStatus

for linux:

.. doxygenclass:: CSockCanScan 
   :project: CanModule
   :members: getPortStatus

errors
------
Errors and problems are available through two mechanisms:

* LogIt. This reports runtime errors detected by CanModule: 
  set the LogIt trace level to TRC and grep for "systec", for component "CanModule"
   
   
**windows**
* vendor and hardware specific errors are available through connection of
  an error handler (to a boost signal carrying the error message, see standard API).
  
.. doxygenclass:: STCanScan 
   :project: CanModule
   :members: STcanGetErrorText

**linux**
* the error message from the ioctl call is returned, unfortunately it is rather unspecific.

