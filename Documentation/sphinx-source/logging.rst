=======
Logging
=======

CanModule uses `LogIt`_ for reporting information during runtime. LogIt uses the four components

* CanModule: general messages concerning CanModule itself
* CanModuleAnagate: messages related to AnaGate modules
* CanModuleSystec: messages related to SysTec modules
* CanModulePeak: messages related to Peak modules

for managing logging levels per vendor. The logging level of each component, if the component is used, can be set individually 
at any time once initialized. For windows the component names are as listed above, for linux the component CanModuleSock is used 
for Systec and Peak modules, but also both CanModuleSystec and CanModulePeak are mapped to CanModuleSock for convenience. 

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
	 * LogIt component CanModule for generic Canmodule
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
	string parameters = "Undefined"; // here: use defaults
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
	Log::LogComponentHandle anagateHandle;
	logIt->getComponentHandle(CanModule::LogItComponentNameAnagate, anagateHandle );
	Log::setComponentLogLevel( anagateHandle, loglevel );
	LOG(Log::INF, anagateHandle) << " logging for component " << CanModule::LogItComponentNameAnagate;

	#ifdef _WIN32
	
	Log::LogComponentHandle stHandle;
	logIt->getComponentHandle(CanModule::LogItComponentNameSystec, stHandle );
	Log::setComponentLogLevel( stHandle, loglevel );
	LOG(Log::INF, stHandle) << " logging for component " << CanModule::LogItComponentNameSystec;

	Log::LogComponentHandle pkHandle;
	logIt->getComponentHandle(CanModule::LogItComponentNamePeak, pkHandle );
	Log::setComponentLogLevel( pkHandle, loglevel );
	LOG(Log::INF, pkHandle) << " logging for component " << CanModule::LogItComponentNamePeak;
	
	#else
	// for linux we can also just use LogItComponentNameSystec and LogItComponentNamePeak 
	Log::LogComponentHandle sockHandle;
	logIt->getComponentHandle(CanModule::LogItComponentNameSock, sockHandle );
	Log::setComponentLogLevel( sockHandle, loglevel );
	LOG(Log::INF, sockHandle) << " logging for component " << CanModule::LogItComponentNameSock;
	
	#endif

 
.. _LogIt: https://github.com/quasar-team/LogIt
