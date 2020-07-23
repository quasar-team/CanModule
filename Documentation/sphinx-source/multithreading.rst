==============
Multithreading
==============

CanModule supports running several bridges from several vendors...
- how many instances per vendor
- port access threadsafe, also for reconnection
- reconnection per port is tried first, and if "shared = false" and port-reset did not re-establish the connection then
the whole bridge is reset. This might work in some cases, depending on the situation and on the vendor's API implementation.
- if bridge reset is also not working then it is advised to re-create the Canmodule-library instance from the server, 
and implicitly re-load the library.  

