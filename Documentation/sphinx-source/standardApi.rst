============
Standard API
============

The user API **hides all implementation/vendor specific details** across implementations (vendors X OSes).


example snippet
---------------

This is how to choose an implementation and open one port:

.. code-block:: c++

   string implementationName = "sock";   // here: systec or peak through socketCan, linux
   string port = "sock:can0";            // here: CAN port 0 via socket CAN, linux
   string parameters = "Unspecified";    // here: use HW settings if possible. see documentation for each implementation/vendor. same as ""
   CanMessage cm;
    =  = CanModule::CanLibLoader::createInstance( implementationName );
   cca = libloader->openCanBus( port, parameters );
   
   cca->setReconnectBehavior( CanModule::ReconnectAutoCondition::sendFail, CanModule::ReconnectAction::singleBus );
   cca->canMessageCame.connect( &onMessageRcvd ); // connect a reception handler 
   cca->canMessageError.connect( &errorHandler ); // connect an error handler
   
   unsigned int br = cca->getPortBitrate();       // make sure we know the bitrate as it was set
  
   cca->sendMessage( &cm );
   
   CanStatistics stats;
   cca->getStatistics( stats );
   std::cout 
         << " timeSinceOpened= " << stats.timeSinceOpened() << " ms"
         << " timeSinceReceived= " << stats.timeSinceReceived() << " ms"
         << " timeSinceTransmitted= " << stats.timeSinceTransmitted() << " ms"
         << " stats portStatus= 0x" << hex << stats.portStatus()
         << " unified port status= 0x" << hex << cca->getPortStatus() << dec << std::endl;
   
   (... communicate...)
      
   libLoader->closeCanBus( cca );
   
   
* Only two strings, "port" and "parameters", have to defined to communicate with a CAN port for a module from a vendor.
* a connection handler method must be registered to treat received messages (boost slot connected to boost signal)
* the openCanBus() is a wrapper which loads the library and opens a can bus with default values. It exists for backward compatibility
* a more direct way is to open the library and then use createBus(..), using also the overloaded, and optionally, frame rate limitation factor or flag:

.. doxygenclass:: CanModule::CCanAccess 
   :project: CanModule
   :members: createBus



.. code-block:: c++

   // the recommended sequence
   CanModule::CanLibLoader *libloader = CanModule::CanLibLoader::createInstance( systecLibName );
   cca->createBus( port.c_str(), parameters.c_str(), fact );


.. code-block:: c++

   // connection.h
   class MYCLASS {
      (...)
      public: 
         static void onMessageRcvd(const CanMsgStruct/*&*/ message); 
   }
   

.. code-block:: c++

   // connection.cpp
   /* static */ void MYCLASS::onMessageRcvd(const CanMsgStruct/*&*/ message){
      MYCLASS *myObject = MYCLASS::getMyObject( ... );
      myObject->processReceivedMessage( message );
   }


.. code-block:: c++

   // errorHandler.cpp
   /* static */ void MYCLASS::errorHandler(const int, const char* msg, timeval/*&*/){
   std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
         << " " << msg << std::endl;
   }
 
   

   
* both the library object **libloader** and the port objet(s) **cca** must exist during runtime, since the **libloader**
  is needed at the end to close the **cca** .
* you can take a look at `CANX`_ for a full multithreaded example using CanModule (CERN, gitlab).

   
