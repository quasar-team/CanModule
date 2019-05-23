===========================
CANX overview and objective
===========================

CANX is a multithreaded "CAN message player and recorder" which uses CanModule and therefore interacts
with all CAN bridges which are supported by CanModule. CANX is available for all supported target platforms,
notably cc7, windows 2016 server and windows 10 enterprise. The sources and cross-system build chain are 
available ((c) CERN). CANX is "a client" to CanModule, much in the same way as any OPC-UA server would be. 

CanModule is a software abstraction layer, written in C++, to simplify integration
of CAN bridges from different vendors into cmake (C++) projects needing CAN connectivity
for windows and linux. A CAN bridge is - usually - an external module which is connected
on one side to a computer or network and offers CAN ports on the other side where CAN buses
can be connected.


CERN CentOs7 cc7 and Windows 2016 Server are supported. 
CAN bridges from Vendors Anagate, Systec and Peak are supported.
CanModule can be source-cloned from gitlab.cern.ch and integrated into a cmake build chain.
For other build chains the binaries and libs can be made available.  

