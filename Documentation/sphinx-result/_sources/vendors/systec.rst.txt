=========
`SysTec`_
=========

All modules from vendor `SysTec`_ are handled by class STCanScan (windows) or CSockCanScan (linux) which 
manage the modules through their underlying vendor specific API and provide the standard generic CanModule API.
Here the underlying vendor specific classes and the specific parameters are documented. 


The connection 
--------------

To connect to a specific port for I/O, and send CAN messages, the following methods are used.

windows
^^^^^^^

The connection to a specific port for I/O is created by calling

.. doxygenclass:: STCanScan
	:project: CanModule
	:members: createBus, sendMessage

and communication takes place through systec's closed-source windows library.
	
linux
^^^^^

The open-source socketcan interface is used on top of systec's open source driver:

.. doxygenclass:: CSockCanScan 
	:project: CanModule
	:members: createBus, sendMessage


sockets are used normally, using linux' built-in CAN protocols:

.. code-block:: c++ 

 mysock = socket(domain=PF_CAN, type=SOCK_RAW, protocol=CAN_RAW)
	

standard CanModule API example
------------------------------

This is how the CanModule standard API is used for systec for linux.

.. code-block:: c++

 libloader = CanModule::CanLibLoader::createInstance( "sock" ); // linux, use "st" for windows
 cca = libloader->openCanBus( "sock:can0", "250000" ); // termination on frontpanel
 CanMessage cm; // empty
 cca->sendMessage( &cm );
