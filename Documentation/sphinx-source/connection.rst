============
Reconnection
============

CanModule tries to recuperate lost connections automatically, but presently there is no method
to interrogate CanModule on the status of it's connected modules. This might be added later, though.
There are up to four ways how to loose a connection, depending on the vendor and type of the module:

(1) power loss
(2) network or USB loss/interruption
(3) firmware or "full" module reset through API
(4) software close/open CAN bus WITHOUT reloading the library.

in short: please avoid (4) and reload the library if possible.

anagate
=======

The anagate modules are easily and uniquely identified by their IP address, also several modules 
per CanModule instance are straightforward. The firmware reset (3) is NOT implemented in the CanModule,
but a module can be remotely accessed and firmware reset using the vendor API, through code like: 

.. code-block:: c++

	AnaInt32 timeout = 12000; // 12secs
	AnaInt32 anaRet = CANRestart( "192.168.1.10", timeout );

In the cases (1),(2) CanModule detects a failure to send a message on a port, and tries to reconnect 
all ports of that module. Sending messages are buffered for ~20secs, and the reconnection 
takes at least ~20sec, so it takes ~1 minute to reestablish communication. All received CAN frames 
are lost, and not all sent frames are guaranteed, therefore some care has to be taken when the
connection is reestablished concerning the statuses of CAN slaves. CanModule reports reconnections
as warnings, but there is no systematic CanModule-API yet to handle reconnections 
systematically (is it needed?). 

The anagate duo reconnects somewhat faster than the X4/X8 modules, because of firmware differences.
The whole reconnection can take up to 60 secs until all buffers are cleared, so please be patient.     

The case (4), software close/open, leads to a deallocation of the connection object, followed by a newly
created connection. These objects are recorded in a (anagate-global) connection map. The library 
load object (see standard API) can also be deallocated and recreated, but it may also be reused.
Although the library does not need to be reloaded, strictly speaking, it is recommended to reload it 
before a (re-)connect:

.. code-block:: c++

		libloader = CanModule::CanLibLoader::createInstance( "an" );
        cca = libloader->openCanBus( "an:can0:xxx.xxx.xxx.xxx", "125000 0 0 0 0"); 
 
WARNING: the Anagate modules tend to firmware-crash if too many CAN bus software close/open are 
executed too quickly, making a full power-cycle of the module necessary. A delay of 7 seconds 
between a close and a (re-)open per module is therefore imposed by CanModule to avoid 
"firmware-crashing" of the anagate module. This crash occurs independently from the connection 
timeout. 

A bigger delay is recommended if it can be afforded: lab-tests show a 7sec delay still crashes 
after 30 consecutive reconnects. These crashes can also be related to networking problems but 
they are difficult to fix.


peak
====

linux/cc7
---------
- power loss is recuperated correctly: the module is receiving power through the USB port, 
if this connection is lost we need to reconnect. Reconnection works for both normal (fixed) 
and flexible datarate (FD) modules under linux, as socketcan is used. 
- Reconnection takes less than 30sec.
- A software close/open is fully supported and works under cc7 without limitations.

windows
-------
- power loss is recuperated correctly, but only normal datarate (fixed) are supported 
- the sw close/open will typically work several times, and CanModule tries to
recuperate from a failed initialisation of the USB 10 times. Between successive attempts on a 
given port a delay of several seconds is needed. This is not great, maybe further progress
can be made later.   

systec
======

linux/cc7
---------
- A power loss or a connection loss will trigger a reconnection. For linux, where socketcan is used,
this works in the same way as for peak. Connections are managed in a global map. 
- A software close/open is fully supported and works under cc7 and also windows without limitations. 
If the sequence is too fast (for the environment..) some messages will be lost, but the 
module recuperates correctly in the following.  


windows
-------
- the hardware reconnection is NOT WORKING, and it is not clear if it can actually
be achieved within CanModule. It seems that a library reload is needed to make the module work again.
This feature is therefore DROPPED for now, since also no strong user request for "systec reconnection
under windows" is presently stated. I tried, using the systec API@windows as documented, but did not manage.

- the software close/open works correctly, some messages can be lost.

