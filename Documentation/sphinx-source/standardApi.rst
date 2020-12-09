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

* The **access to a CAN port** is through:

.. doxygenclass:: CanModule::CCanAccess
	:project: CanModule
	:members: openCanBus, closeCanBus


* This **snippet** gives an overview how the API is used to hide vendor details and achieve CAN connectivity:

.. code-block:: c++

	string implementationName = "sock";   // here: systec or peak through socketCan, linux
	string port = "sock:can0";            // here: CAN port 0 via socket CAN, linux
	string parameters = "Unspecified";    // here: use defaults. see documentation for each implementation/vendor. same as ""
	CanMessage cm;
	CanModule::CanLibLoader *libloader = CanModule::CanLibLoader::createInstance( implementationName );
	cca = libloader->openCanBus( port, parameters );
	cca->sendMessage( &cm );
	cca->canMessageCame.connect( &onMessageRcvd ); // connect a reception handler 
   unsigned int br = cca->getPortBitrate();       // make sure we know the bitrate as it was set
   (... communicate...)
   libLoader->closeCanBus( cca );
	
	
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
	

read back the port bitrate to be sure:

.. doxygenclass:: CanModule::CCanAccess
   :project: CanModule
   :members: getPortBitrate



* you can take a look at `CANX`_ for a full multithreaded example using CanModule (CERN, gitlab).
Contact me for a simple code example.

.. _CANX: https://gitlab.cern.ch/mludwig/canx.git


