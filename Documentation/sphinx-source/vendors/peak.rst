=======
`Peak`_
=======

All modules from vendor `Peak`_ are handled by class PKCanScan (windows) or CSockCanScan (linux) which 
both manage the modules through their underlying vendor specific API according to the OS. 
Both classes provide the standard generic CanModule API. 

The modules from the families PCAN USB and USB Pro are supported. For linux, also the flexible 
datarate (FD) module types are supported, but without FD functionality. This FD functionality 
could be added in principle if requested.


windows
^^^^^^^
The communication takes place through peak's open-source PCAN-Basic windows library. Only "plug-and-play"
modules with USB interface and fixed datarate (non-FD) are supported by CanModule for now. PEAK's 
flexible datarate (FD) modules can be added later on (they need some different API-calls and more complex parameters), and also
other interfaces like PCI are possible for windows.The implementation is based on the PCAN-Basic driver.

linux
^^^^^
The open-source socketcan interface is used on top of peak's open source netdev driver. Both Peak's
fixed and flexible datarate are working, although the fixed modules are recommended for bus compatibility.
Only modules with USB interface are supported. 
The peak driver source is freely available and it can be configured to build several
types of drivers, where we use peak's netdev driver only. See `PeakDriver`_ for details on this.
A PCAN-Basic driver is also available but the netdev driver is more performant and modern. The 
PCAN-Basic driver is used nevertheless for windows, and it offers better compatibility for all module
families. 

Unfortunately the peak linux driver does NOT provide deterministic port numbering through socketcan. On a 
system with several PEAK bridges the port specification "sock:can0" is mapped to **any**
of the existing peak bridges (and the electronics can burn because you don't know which 
can-bus you are connected to !). In order to overcome this limitation for peak
(it really is a design error in their driver) an extended port identifier must be used: "sock:can0:device1234".

CanModule then performs a udev system call and internally remaps the ports, so that "sock:can0:device1234" 
will correspond to the first port of device 1234. The internal mapping is deterministic since the devices
can always be ordered ascending. 
In order to use the deviceID ("1234") it has to be set persistently for each module, using the windows 
utility peakCan. Device ID numbers have to be unique in a system obviously. The assigning of port numbers
is then done in ascending order: the first batch of ports is from the device with the lowest device ID, 
and so forth.

.. doxygenclass:: udevanalyserforpeak_ns::UdevAnalyserForPeak 
   :project: CanModule
   :members: 

status
------
status information is propagated through the unified status.
For windows:

.. doxygenclass:: PKCanScan 
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
  set the LogIt trace level to TRC and grep for "peak", for component "CanModule"
   
   
**windows**
* vendor and hardware specific errors are available through connection of
  an error handler (to a boost signal carrying the error message, see standard API).
  The messages and error codes originate from the vendor api/implementation and are
  reported as is without further processing. Messages are taken from the vendor's API
  documentation if available. Peak relies on a vendor provided method: CAN_GetErrorText.

**linux**
* the error message from the ioctl call is returned, unfortunately it is rather unspecific.

.. _PeakDriver: https://readthedocs.web.cern.ch/display/CANDev/CAN+development?src=sidebar


