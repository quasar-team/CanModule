==========
`AnaGate`_
==========

All modules from vendor `AnaGate`_ are handled by class AnaCanScan which manages the modules through their underlying vendor specific API and provides 
the standard generic CanModule API. 

We support Anagate CAN-ethernet gateways: uno, duo, quattro, and the X2, X4 and X8.
 
Since these modules communicate to the host computer only via ethernet, at the fundamental level only classical 
tcp/ip ethernet is needed. Nevertheless the specific contents of the IP frames are wrapped up in an Anagate API for convenience, which is linked
into the user code as a library. There are therefore no implementation differences between Linux and Windows.    

The downside of Anagate CAN-ethernet modules is of course that the latency of the network has to be added to the bridge latency. 

status
------
status information is propagated through the unified status.

.. doxygenclass:: AnaCanScan 
   :project: CanModule
   :members: getPortStatus

errors
------
Errors and problems are available through two mechanisms:

* LogIt. This reports runtime errors detected by CanModule: 
  set the LogIt trace level to TRC and grep for "anagate", for component "CanModule"
   
* vendor and hardware specific errors are available through connection of
  an error handler (to a boost signal carrying the error message, see standard API).
  The messages and error codes originate from the vendor api/implementation and are
  reported as is without further processing. Messages are taken from the vendor's API
  documentation if available.

.. doxygenclass:: AnaCanScan 
   :project: CanModule
   :members: ana_canGetErrorText
