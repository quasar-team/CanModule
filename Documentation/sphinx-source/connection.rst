==========
Connection
==========

CanModule tries to recuperate lost connections automatically, but presently there is no method
to interrogate CanModule on the status of it's connected modules. This might be added later, though.

anagate
-------
The anagate modules are easily and uniquely identified by their IP address, also several modules 
per CanModule instance are straightforward. An anagate bridge can be disconnected in 3 ways:

* power loss - power back
* network interruption
* firmware reset by software through the API

the firmware reset is NOT implemented in the CanModule, but a module can be remotely accessed and 
firmware reset using the vendor API, through code like: 

.. code-block:: c++

	AnaInt32 timeout = 10000; // 10secs
	AnaInt32 anaRet = CANRestart( "192.168.1.10", timeout );


In all 3 cases CanModule detects a failure to send a message on a port, and tries to reconnect 
all ports of that module. Sending messages are buffered for ~20secs, and the reconnection 
takes at least ~20sec, so it takes ~1 minute to reestablish communication. All received CAN frames 
are lost, and not all sent frames are guaranteed, therefore some care has to be taken when the
connection is reestablished concerning the statuses of CAN slaves. CanModule reports reconnections
as warnings, but there is no systematic CanModule-API yet to handle reconnections 
systematically (is it needed?). 
	The anagate duo reconnects somewhat faster than the X4/X8 modules, because of firmware differences.
The whole reconnection can take up to 60 secs until all buffers are cleared, so please be patient.     

peak and systec
---------------

The reconnection behavior is still under investigation and testing.
