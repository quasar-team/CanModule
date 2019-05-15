=========
`SysTec`_
=========

All modules from vendor `SysTec`_ are handled by class STCanScan (windows) or CSockCanScan (linux) which 
manage the modules through their underlying vendor specific API and provide the standard generic CanModule API. 

* **windows:** The connection to a specific port for I/O is created by calling

.. doxygenclass:: STCanScan
	:project: CanModule

and communication takes place through systec's closed-source windows library.
	
* **linux:** The open-source socketcan interface can be used on top of systec's open source driver. The class
	
.. doxygenclass:: CSockCanScan 
	:project: CanModule

is called and opens a socket::
 
	mysock = socket(domain=PF_CAN, type=SOCK_RAW, protocol=CAN_RAW)
	
for each connection.


Systec Parameters
-----------------

**string::port**

* "st:<port>"
where <port> is the number of the CAN port on the module, 0...N, separated by ":" 
example: "st:0" for systec CAN port 0

**string::parameters**
 
* "Unspecified" : using defaults

* "p0" : usually a list of parameters separated by whitespace, but for systec only p0 
p0: baudrate in kbit/s { 50000, 100000, 125000, 250000, 500000, 1000000 }

example: "250000"



 
