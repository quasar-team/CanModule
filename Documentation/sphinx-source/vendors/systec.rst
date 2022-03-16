=========
`SysTec`_
=========

All modules from vendor `SysTec`_ are handled by class STCanScan (windows) or CSockCanScan (linux) which 
manage the modules through their underlying vendor specific API and provide the standard generic CanModule API.
Here the underlying vendor specific classes and the specific parameters are documented. 

SysTec modules USB-CAN bridges are supported: sysWORXX 1,2,8,16


linux
^^^^^

parameters
----------

.. doxygenclass:: CSockCanScan  
   :project: CanModule
   :members: configureCanBoard, parseNameAndParameters
   :private-members: 
   :no-link:

* bus name: "can" or "vcan"
* port number, follows bus name: "can:0" or "vcan:9"

* parameters="p0 p1 p2 p3 p4 p5 p6" i.e. "125000 0 0 0 0 0 5000"
* ONLY p0 = bitrate or "baudrate" is used.
* p0 = m_lBaudRate
* "Unspecified" : the socket bitrate is NOT reconfigured , no stop/start sequence on the socket are done


windows
^^^^^^^

parameters
----------

.. doxygenclass:: STCanScan  
   :project: CanModule
   :members: configureCanBoard, parseNameAndParameters
   :private-members: 
   :no-link:

* parameters="p0 p1 p2 p3 p4 p5 p6" i.e. "125000 0 0 0 0 0 5000"
* ONLY p0 = bitrate or "baudrate" is used.
* "Unspecified" : the module bitrate 125000 is used as default when calling openCanBoard(). The init parameters are quite involved abut only the bitrate is relevant.


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

