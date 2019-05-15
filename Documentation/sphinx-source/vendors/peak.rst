=======
`Peak`_
=======

All modules from vendor `Peak`_ are handled by class PKCanScan (windows) or CSockCanScan (linux) which 
both manage the modules through their underlying vendor specific API according to the OS. 
Both classes provide the standard generic CanModule API. 

* **windows:** the connection to a specific port for I/O is created by calling

.. doxygenclass:: PKCanScan 
	:project: CanModule
	:members: createBus, sendMessage
	
and communication takes place through peak's open-source PCAN-Basic windows library.

* **linux:** The open-source socketcan interface 
is used on top of peak's open source netdev driver. The class

.. doxygenclass:: CSockCanScan 
	:project: CanModule
	:members: createBus, sendMessage
	
is called and opens a socket::
 
	mysock = socket(domain=PF_CAN, type=SOCK_RAW, protocol=CAN_RAW)
	
for each connection. 
	

Peak Parameters
-----------------

**string::port**

* for windows: "pk:<port>"
* for linux:   "sock:<port>"

where <port> is the number of the CAN port on the module, 0...N, separated by ":"
 
example: **"pk:0"** (or also "pk:can0") for peak CAN port 0, for windows

example: **"sock:1"** (or also "sock:can1") for peak CAN port 1, for linux

**string::parameters**
 
* "Unspecified" : using defaults, bitrate=125000 bit/s

* "p0" : usually a list of parameters separated by whitespace, but for peak only one parameter p0 

p0: baudrate in bit/s { 50000, 100000, 125000, 250000, 500000, 1000000 }

**example:**

*   libloader = CanModule::CanLibLoader::createInstance( "sock" );
*   cca = libloader->openCanBus( "sock:can0", "250000" );

