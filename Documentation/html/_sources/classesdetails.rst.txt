==================
Details on classes
==================

* An **implementation is chosen** and specific details are then taken care of by loading any further dependencies:
   
.. doxygenclass:: CanModule::CanLibLoader
   :project: CanModule
    
.. doxygenclass:: CanModule::CanLibLoaderLin
   :project: CanModule
 
.. doxygenclass:: CanModule::CanLibLoaderWin
   :project: CanModule


* Connection/access details to the **different implementations/vendors** are managed by sub-classing:

.. doxygenclass:: CanModule::CCanAccess 
   :project: CanModule
 
.. doxygenclass:: AnaCanScan 
   :project: CanModule
   :members: sendMessage, CanMessageCame, getStatistics, getPortStatus

.. doxygenclass:: CSockCanScan 
   :project: CanModule
   :members: sendMessage, CanMessageCame, getStatistics, getPortStatus

.. doxygenclass:: PKCanScan 
   :project: CanModule
   :members: sendMessage, CanMessageCame, getStatistics, getPortStatus

.. doxygenclass:: STCanScan 
   :project: CanModule
   :members: sendMessage, CanMessageCame, getStatistics, getPortStatus


tracing connections
-------------------
 
* keep trace of all connections to buses using a static API class without instances:

.. doxygenclass:: CanModule::Diag 
   :project: CanModule
   :members: get_connections
   
