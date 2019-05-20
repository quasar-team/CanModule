=======
`Peak`_
=======

All modules from vendor `Peak`_ are handled by class PKCanScan (windows) or CSockCanScan (linux) which 
both manage the modules through their underlying vendor specific API according to the OS. 
Both classes provide the standard generic CanModule API. 
Here the underlying vendor specific classes and the specific parameters are documented. 

The connection 
--------------

To connect to a specific port for I/O, and send CAN messages, the following methods are used.

windows
^^^^^^^

the connection to a specific port for I/O is created by calling

.. doxygenclass:: PKCanScan 
	:project: CanModule
	:members: createBus, sendMessage
	
and communication takes place through peak's open-source PCAN-Basic windows library. Only "plug-and-play"
modules with USB interface and fixed datarate are supported by CanModule for now. PEAK's flexible datarate (FD)
modules can be added later on (they need some different API-calls and more complex parameters), and also
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

.. doxygenclass:: CSockCanScan 
	:project: CanModule
	:members: createBus, sendMessage
	

sockets are used normally, using linux' built-in CAN protocols:

.. code-block:: c++ 

 mysock = socket(domain=PF_CAN, type=SOCK_RAW, protocol=CAN_RAW)


standard CanModule API example
------------------------------

This is how the CanModule standard API is used for peak for windows.

.. code-block:: c++

 libloader = CanModule::CanLibLoader::createInstance( "pk" ); // windows, use "sock" for linux
 cca = libloader->openCanBus( "pk:can0", "250000" ); // termination on frontpanel
 CanMessage cm; // empty
 cca->sendMessage( &cm );


.. _PeakDriver: https://readthedocs.web.cern.ch/display/CANDev/CAN+development?src=sidebar


