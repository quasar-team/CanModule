=======
`Peak`_
=======

All modules from vendor `Peak`_ are handled by class PKCanScan which manages the modules through their underlying vendor specific API and provides 
the standard generic CanModule API. 

The connection to a specific port for I/O is created by calling

.. doxygenclass:: PKCanScan 
	:project: CanModule
	:members: createBus, sendMessage

.. doxygenclass:: CSockCanScan 
	:project: CanModule
	:members:
	:undoc-members:

.. doxygenclass:: MockCanAccess 
	:project: CanModule

