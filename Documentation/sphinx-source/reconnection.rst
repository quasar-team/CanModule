============================================
Reconnection OBSOLETE IS REWRITTEN CURRENTLY
============================================


anagate
=======

The anagate modules are identified by their IP address. 

Sending messages are buffered for ~20secs, and the reconnection 
takes at least ~20sec, so it takes ~1 minute to reestablish communication. All received CAN frames 
are lost, and not all sent frames are guaranteed, therefore some care has to be taken when the
connection is reestablished concerning the statuses of CAN slaves. 

The anagate duo reconnects somewhat faster than the X4/X8 modules, because of firmware differences.
The whole reconnection can take up to 60 secs until all buffers are cleared, so please be patient.     
 
WARNING: the Anagate modules tend to firmware-crash if too many CAN bus software close/open are 
executed too quickly, making a power-cycle of the module necessary. A delay of 7 seconds 
between a close and a (re-)open per module is a good idea to avoid 
"firmware-crashing" of the anagate module (CanModule does not impose such a delay).
This crash occurs independently from the connection timeout. 

A bigger delay is recommended if it can be afforded: lab-tests show a 7sec delay still crashes 
after 30 consecutive reconnects. These crashes can also be related to networking problems but 
they are difficult to fix.


peak
====

linux/cc7
---------
A power loss is recuperated correctly: the module is receiving power through the USB port, 
if this connection is lost we need to reconnect. Reconnection works for both normal (fixed) 
and flexible datarate (FD) modules under linux, as socketcan is used, and takes less than 30sec.

windows
-------
A power loss is recuperated correctly, but only normal datarate (fixed) are supported. 
The single port close/open will typically work several times, and CanModule tries to
recuperate from a failed initialisation of the USB 10 times. Between successive attempts on a 
given port a delay of several seconds is needed. This is not great, maybe further progress
can be made later.   

systec
======

linux/cc7
---------
A power loss or a connection loss will trigger a reconnection. For linux, where socketcan is used,
this works in the same way as for peak. Single port close/open is fully supported and works under 
cc7 and also windows without limitations. If the sequence is too fast some messages will be lost, but the 
module recuperates correctly in the following.  


windows
-------
The whole module reconnection is NOT WORKING, and it is not clear if it can actually
be achieved within CanModule. It seems that a library reload is needed to make the module work again.
This feature is therefore DROPPED for now, since also no strong user request for "systec whole module reconnection
under windows" is presently stated. I tried, using the systec API@windows as documented, but did not manage.

Single port close/open works correctly, some messages can be lost.

