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
   string parameters = "Unspecified";    // here: use defaults. see documentation for each implementation/vendor. same as ""
   CanMessage cm;
   CanModule::CanLibLoader *libloader = CanModule::CanLibLoader::createInstance( implementationName );
   cca = libloader->openCanBus( port, parameters );
   
   cca->setReconnectBehavior( CanModule::ReconnectAutoCondition::sendFail, CanModule::ReconnectAction::singleBus );
   cca->canMessageCame.connect( &onMessageRcvd ); // connect a reception handler 
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

.. code-block:: c++

   // connection.h
   class CONNECTION {
      (...)
      public: 
         static void onMessageRcvd(const CanMsgStruct/*&*/ message); 
   }
   

.. code-block:: c++

   // connection.cpp
   /* static */ void CONNECTION::onMessageRcvd(const CanMsgStruct/*&*/ message){
      MYCLASS *myObject = MYCLASS::getMyObject( ... );
      myObject->processReceivedMessage( message );
   }
   

* you can take a look at `CANX`_ for a full multithreaded example using CanModule (CERN, gitlab).

   
