=======
Logging
=======

CanModule uses `LogIt`_ for reporting information during runtime. LogIt uses one component "CanModule"

* CanModule: general messages concerning CanModule itself
* CanModule Anagate: messages related to AnaGate modules contain a string "anagate"
* CanModule Systec: messages related to SysTec modules contain a string "systec"
* CanModule Peak: messages related to Peak modules contain a string "peak"

for managing logging levels. The logging level of each component, if the component is used, can be set individually 
at any time once initialized. Vendor specific messages can be filtered out by using the specific strings.
For windows the strings are as listed above, for linux the string "sock" is used for Systec and Peak modules. 

You can of course add your own components for specific logging, like MYCOMP in the code below.

**The calling program ("main") uses CanModule and LogIt like this**

.. code-block:: c++

	#include <CanBusAccess.h>
	#include <LogIt.h>
	...
	Log::LogComponentHandle myHandle = 0;
	Log::LogComponentHandle canModuleHandle = 0;
	Log::LOG_LEVEL loglevel = Log::INF;    // recommended default for production is WRN
	bool ret = Log::initializeLogging( loglevel );
	if ( ret ) 
		cout << "LogIt initialized OK" << endl;
	else 
		cout << "LogIt problem at initialisation" << endl;
	Log::setNonComponentLogLevel( loglevel );
	std::cout << "Log level set to " << loglevel << endl;
	LogItInstance *logIt = LogItInstance::getInstance();

	/**
	 * LogIt component MYCOMP for main
	 */
	Log::registerLoggingComponent( "MYCOMP", loglevel );
	logIt->getComponentHandle( "MYCOMP", myHandle );
	Log::setComponentLogLevel( myHandle, loglevel );
	LOG(Log::INF, myHandle) << argv[ 0 ] << " logging for component MYCOMP"; 
	// hourray, we should see this message because we are at INF

	/**
	 * LogIt component CanModule
	 */
	Log::registerLoggingComponent( CanModule::LogItComponentName, loglevel );
	logIt->getComponentHandle(CanModule::LogItComponentName, canModuleHandle );
	Log::setComponentLogLevel( canModuleHandle, loglevel );
	LOG(Log::INF, canModuleHandle) << " logging for component " << CanModule::LogItComponentName;

**then, some work is done i.e. like that:**


.. code-block:: c++

	// do sth useful with CanModule, i.e. talk to a port
	string libName = "st";           // here: systec, windows
	string port = "st:can0";         // here: CAN port 0, windows
	string parameters = "Unspecified"; // here: use what is alread set in the hardware, do not overwrite. Therefore the CanModule does not really know.
	CanMessage cm;
	CanModule::CanLibLoader *libloader = CanModule::CanLibLoader::createInstance( libName );
	cca = libloader->openCanBus( port, parameters );
	cca->sendMessage( &cm );
	cca->canMessageCame.connect( &onMessageRcvd ); // connect a reception handler, see standardApi for details 
	

**and at any time the logging levels of the components can be changed. This is typically done by a user interaction 
on a running server instance.**
 	

.. code-block:: c++

	/**
	 * manipulate LogIt levels per component for testing, during runtime. Set loglevel to TRC (max verbosity)
	 */
	loglevel = LOG::TRC;
	Log::LogComponentHandle handle;
	logIt->getComponentHandle(CanModule::LogItComponentName, handle );
	Log::setComponentLogLevel( handle, loglevel );
	LOG(Log::INF, handle) << " logging for component " << CanModule::LogItComponentName;

 
.. _LogIt: https://github.com/quasar-team/LogIt
