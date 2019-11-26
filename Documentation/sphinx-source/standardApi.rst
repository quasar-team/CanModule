============
Standard API
============

The user API is using **3 classes and hides all vendor specific details**.
Only a few common methods are needed:

* A **vendor is chosen** and specific details are then taken care of by loading any further dependencies:
	
.. doxygenclass:: CanModule::CanLibLoader
	:project: CanModule

* Connection/access details to the **different vendors** are managed by sub-classing:

.. doxygenclass:: CanModule::CCanAccess 
	:project: CanModule
	

CAN ports
---------

CAN ports are specified by the user, and in the strict sense, a port is an integer number 0..N.
Vendors and implementations tend to handle that differently, but CanModule tries to provide a
standard API across all vendors. The following strings, specified for CAN ports, will connect to port X:

* "X"
* "canX"
* "vcanX"
* "moduleX"
* "whateverX"

but "module2PortX" will connect to 2 instead of port X, so please stay reasonable.

* The **access to a CAN port** is through:

.. doxygenclass:: CanModule::CanBusAccess
	:project: CanModule
	:members: openCanBus, closeCanBus


* This **snippet** gives an overview how the API is used to hide vendor details and achieve CAN connectivity:

.. code-block:: c++

	string libName = "sock";         // here: systec or peak through socketCan, linux
	string port = "sock:can0";       // here: CAN port 0 via socket CAN, linux
	string parameters = "Undefined"; // here: use defaults
	CanMessage cm;
	CanModule::CanLibLoader *libloader = CanModule::CanLibLoader::createInstance( libName );
	cca = libloader->openCanBus( port, parameters );
	cca->sendMessage( &cm );
	cca->canMessageCame.connect( &onMessageRcvd ); // connect a reception handler 
	
	
* Only two strings, "port" and "parameters"

have to defined to communicate with a CAN port for a module from a vendor.

* a connection handler method

must be registered to treat received messages (boost slot connected to boost signal):

.. code-block:: c++

	connection.h :
	class CONNECTION {
	   (...)
	   public: 
	      static void onMessageRcvd(const CanMsgStruct/*&*/ message); 
	}
	

.. code-block:: c++

	connection.cpp :
	/* static */ void CONNECTION::onMessageRcvd(const CanMsgStruct/*&*/ message){
	   MYCLASS *myObject = MYCLASS::getMyObject( ... );
	   myObject->processReceivedMessage( message );
	}
	

* you can take a look at `CANX`_ for a full multithreaded example using CanModule (CERN, gitlab).
Contact me for a simple code example.

.. _CANX: https://gitlab.cern.ch/mludwig/canx.git


