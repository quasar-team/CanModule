============================
standard API list of methods
============================


.. doxygenclass:: CanModule::CCanAccess  
   :project: CanModule
   :members: sendRemoteRequest,createBus,sendMessage,getBusName,getPortStatus,getPortBitrate,getStatistics,setReconnectBehavior,setReconnectReceptionTimeout,setReconnectFailedSendCount,getReconnectCondition,getReconnectAction,stopBus
   :members-only: 

.. doxygenclass:: CanModule::CanStatistics  
   :project: CanModule
   :members: totalTransmitted,totalReceived,txRate,rxRate,busLoad,timeSinceReceived,timeSinceTransmitted,timeSinceOpened
   :members-only:

.. doxygenclass:: CanModule::CanBusAccess  
   :project: CanModule
   :members: openCanBus,closeCanBus
   :members-only:

