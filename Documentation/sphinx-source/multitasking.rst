============
Multitasking
============

CanModule is instantiated for a given vendor and bridge and loads the needed libraries accordingly. That instance is called 
CanModule-library. Each CanModule-library then manages CAN ports on the bridge.

Having CanModule-library instances on different tasks, or even hosts, is of course possible, and each of those "separated" 
instances can manage CAN ports as needed on any supported bridge. There are several restrictions in this case:

reconnection behavior
---------------------
if any of the "separated" Canmodule-library instances decides that something went wrong with a bridge it has to 
 reset that bridge. This will evidently affect all other tasks which use CAN ports on the same bridge. This is highly
 undesired and will often lead to a cyclic reset and undefined behavior across the tasks. In order to avoid this
 a switch has to be set when the CanModule-library is instantiated: "shared = true". The default is "shared = false". 
  
multiple connections
--------------------
if different tasks connect to the same physical CAN port, on the same physical bridge, via two separated Canmodule-library
instances there will be collisions on the CAN port. In order to prevent this CanModule would have to do some complex 
networking between instances, and this is undesired and out of scope. Therefore it is the responsibility of the user
to configure the according OPCUA server(s) (which use CanModule) to avoid this, across the installation.   

Generally it is a very good idea to avoid such bridge sharing in order to profit from the reconnection functionality and
avoid having to restart any server or even bridge. Port sharing is totally bad of course.

**If bridge sharing is really needed then the flag "shared = true" has to be set.**