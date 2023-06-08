========
AnaGate2
========

All modules of CERN prototype from vendor `AnaGate`_ are handled by class AnaCanScan2 which manages the modules through their underlying vendor specific API and provides the standard generic CanModule API. 

We support Anagate2 CAN-ethernet gateways: FZ and FX series > 2020 which run the according firmware ANAGATE_CAN_F_v3-1-0_BETA-2.swu (june2023). This
firmware will become a classical version.

The only difference between anagate and anagate2 is the extended diagnostics. All other functions are identical.
In fact, all implementations, also the ones which DO NOT have any extended diagnostics, have the according methods implemented.
If the hw does not support it the according calls will just return empty strings and values. Like this, any client (OpcUaServer)
can just implement the extended diagnostics fully and rely on CanModule for the results.

   
diagnostics
-----------
The extended diagnostics are available through direct HW access methods. These methods must be invoked by the client, possibly
from inside an error handler.

.. doxygenclass:: AnaCanScan2 
   :project: CanModule
   :members: getHwLogMessages,getHwDiagnostics,getHwCounters





   
   
