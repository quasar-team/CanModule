==========
`AnaGate`_
==========

All modules from vendor `AnaGate`_ are handled by class AnaCanScan which manages the modules through their underlying vendor specific API and provides 
the standard generic CanModule API. 

We support Anagate CAN-ethernet gateways: uno, duo, quattro, and the X2, X4 and X8.
 
Since these modules communicate to the host computer only via ethernet, at the fundamental level only classical 
tcp/ip ethernet is needed. Nevertheless the specific contents of the IP frames are wrapped up in an Anagate API for convenience, which is linked
into the user code as a library. There are therefore no implementation differences between Linux and Windows.    
Here the underlying vendor specific classes and the specific parameters are documented.

The downside of Anagate CAN-ethernet modules is of course that the latency of the network has to be added to the bridge latency. 

The connection
--------------

To connect to a specific port for I/O, and send CAN messages, the following methods are used.

.. doxygenclass:: AnaCanScan 
	:project: CanModule
	:members: createBus, sendMessage

		
standard CanModule API example
------------------------------

This is how the CanModule standard API is used for anagate. The code is identical for linux and windows.

.. code-block:: c++

 libloader = CanModule::CanLibLoader::createInstance( "an" );
 cca = libloader->openCanBus( "an:can0:137.138.12.99", "250000 0 1" ); // termination, ISEG controllers, p3, p4, p5 defaults
 CanMessage cm; // empty
 cca->sendMessage( &cm );
 
 
 
 


