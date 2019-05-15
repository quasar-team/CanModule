===========
`AnaGate`_
===========

All modules from vendor `AnaGate`_ are handled by class AnaCanScan which manages the modules through their underlying vendor specific API and provides 
the standard generic CanModule API. 

The connection to a specific port for I/O is created by calling


.. doxygenclass:: AnaCanScan 
	:project: CanModule
	:members: createBus, sendMessage


 
	