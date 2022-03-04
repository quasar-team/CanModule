=====================
Reconnection Behavior
=====================

The aim of the reconnection behavior is, in case reception or sending of CAN messages shows problems, 
to automatically go through an established reconnection sequence. Both sending and receiving (handler) 
CAN messages should be reestablished within CanModule, so that the client (~OPCUA server) does not
need to care about vendor details for that.

The reconnection behavior must be configured at startup, and can not be changed during CanModule runtime
 (a library reload can be made to get a new instance of CanModule which can be re-configured). 
It specifies:

- a reconnection condition, depending on send or receive faults or timeouts

.. doxygenclass:: CanModule::CCanAccess 
   :project: CanModule
   :members: ReconnectAutoCondition

- a reconnection action which is executed if the condition is true
   
.. doxygenclass:: CanModule::CCanAccess 
   :project: CanModule
   :members: ReconnectAction
 
 
- the reconnection thread is triggered each time a sending or receiving problem is detected. The vendor-specific behavior is implemented in that thread,
therefore these threads are private vendor specific, but with standardised trigger behavior.  

The defaults, which are valid for all vendors, are:

- condition = true after 10 successive send fails
- action =  reset the signle bus where the error occurred

Not all combinations are possible for all vendors and OS, and behavior is generally 
different in each specific case. CanModule tries to hide this as best as possible, so that
trips to the physical hardware can be avoided in many cases. 

The reconnection behavior is tested in the lab by physically disconnecting, then waiting
and reconnecting a CAN bridge while it is sending and receiving. The test passes when 
both sending and receiving reestablish without manual intervention (and without specific client code).

Nevertheless, the client (~OPCUAserver) can choose to implement it's own strategy: for this
the reconnection behavior can be simply switched off completely.

The reconnection behavior is implemented in such a way to provide also a NON BLOCKING senMessage() method.
The whole reconnection code is run in a seperate thread for each bus, and that thread is always alive. It gets
triggered each time there is a sending problem detected, and each time there has been no reception for more than 1 second.
The reconnection thread then decides, according to it's configuration, if there is something to do (condition=it checks 
counters and timeouts) and what to do (action=reset single bus or whole board or nothing).  


anagate ("an")
==============

The anagate modules are identified by one or several IP addresses, depending on the module variant. 
The supported anagates are OS independent since they are eth-CAN bridges.

anagate modules with ONE ip number only
---------------------------------------

Sending messages are buffered for ~20secs, and the reconnection 
takes at least ~20sec, so it takes ~1 minute to reestablish communication. All received CAN frames 
are lost, and not all sent frames are guaranteed, therefore some care has to be taken when the
connection is reestablished concerning the statuses of CAN slaves. 

The anagate duo reconnects somewhat faster than the X4/X8 modules, because of firmware differences.
The whole reconnection can take up to 60 secs until all buffers are cleared, so please be patient.     
 
WARNING: the Anagate modules tend to firmware-crash if too many CAN bus software close/open are 
executed too quickly, making a power-cycle of the module necessary. A delay of 7 seconds 
between a close and a (re-)open per module is a good idea in order to avoid 
"firmware-crashing" of the anagate module (CanModule does not impose such a delay).
This crash occurs independently from the connection timeout. 

A bigger delay is recommended if it can be afforded: lab-tests show a 7sec delay still crashes 
after 30 consecutive reconnects. These crashes can also be related to networking problems but 
they are difficult to fix.

Nevertheless, if enough time is allowed for the firmware to be fully up (30secs) the reconnection 
works beautifully, for single ports and also for the whole module.

multi-ip anagate modules (some 2022 and later)
----------------------------------------------

Concerning the reconnection behavior such modules are treated as submodules per-ip, meaning that a sub-module with one ip
behaves like a single-ip anagate (see above). We have 1_ip<->4_CAN modules, and 1-ip<->8_CAN.

It is technically possible to reset a whole anagate by reloading
it's firmware, or by power cycling, but CanModule **does not** use the correspongding API calls. Keep it simple and safe.
If you want to software-shoot-down-and-reload-firmware a whole module nevertheless 
(and not just power cycle it using a PDU or similar) then I might have some non-CanModule code for you if 
you are brave - but that is not supported for now.


peak
====

linux/cc7 ("sock")
------------------
A power loss is recuperated correctly: the module is receiving power through the USB port, 
if this connection is lost we need to reconnect. Reconnection works for both normal (fixed) 
and flexible datarate (FD) modules under linux, as socketcan is used, and takes less than 30sec.
Correct port numbering is achieved (through a udevadmin call) as well.

windows ("peak")
----------------
A power loss is recuperated correctly, but only normal datarate (fixed) are supported. 
The single port close/open will typically work several times, and CanModule tries to
recuperate from a failed initialisation of the USB 10 times. Between successive attempts on a 
given port a delay of several seconds is needed. This is not great, maybe further progress
can be made later, but I am not optimistic.   

systec
======

linux/cc7 ("sock")
------------------
A USB connection or power loss/recover will trigger a reconnection. 

- This is tested on a systec16 by disconnectiing the USB.  
- It is highly recommended to compile and install the systec driver for the target kernel version. Other versions might insert without error but 
will have various problems later during runtime.
- Single port close/open is fully supported and works. If the sequence is too fast some messages will be lost, but the 
module recuperates correctly in the following. Port numbering is preserved.
- whole module reconnect is not supported due to the socketcan abstraction, which abstracts the concept od a "module" away.

In the case of a power loss and recovery, the driver should be automatically inserted again (dmesg | grep systec), but the network
interfaces need to be re-activated (i.e. ifconfig can0 down; ip link set can0 type can bitrate 125000; ifconfig can0 up) on the OS level.
For production systems using a supervisor script checking the network interfaces, and bringing them up again, is recommended.


.. doxygenclass:: CanModule::CSockCanScan 
   :project: CanModule
   :members: CanReconnectionThread


windows ("systec")
------------------
The whole module reconnection is NOT WORKING, and it is not clear if it can actually
be achieved within CanModule. It seems that a library reload is needed to make the module work again.
This feature is therefore DROPPED for now, since also no strong user request for "systec whole module reconnection
under windows" is presently stated. I tried, using the systec API@windows as documented, but did not manage.

Single port close/open works correctly, some messages can be lost, at least we have that (default).

