============
Standard API
============

The user API is using **3 classes and hides all vendor specific details**.
Only a few common methods are needed:

* A **vendor is chosen** and specific details are then taken care of by loading any further dependencies:
	
.. doxygenclass:: CanModule::CanLibLoader
	:project: CanModule
	:undoc-members:

* Connection/access details to the **different vendors** are managed by sub-classing:

.. doxygenclass:: CanModule::CCanAccess 
	:project: CanModule
	
* The **access to a CAN port** is through:

.. doxygenclass:: CanModule::CanBusAccess
	:project: CanModule
	:members: openCanBus, closeCanBus
	:undoc-members:


* This **snippet** gives an overview how the API is used to hide vendor details and achieve CAN connectivity:

.. code-block:: c++

	string libName = "st";           // here: systec
	string port = "0";               // here: CAN port 0
	string parameters = "Undefined"; // here: use defaults
	CanMessage cm;
	CanModule::CanLibLoader *libloader = CanModule::CanLibLoader::createInstance( libName );
	cca = libloader->openCanBus( port, parameters );
	cca->sendMessage( &cm );
	cca->canMessageCame.connect( &onMessageReceivedHandler ); // connect a reception handler 
	
	