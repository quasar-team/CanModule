/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * AnaCanScan.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: pnikiel, vfilimon, quasar team
 *      Maintainer: mludwig at cern dot ch
 *
 *  This file is part of Quasar.
 *
 *  Quasar is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public Licence as published by
 *  the Free Software Foundation, either version 3 of the Licence.
 *
 *  Quasar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public Licence for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Quasar.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Here the Anagate ethernet-CAN bridges are implemented
 */

#include <iomanip>


#include "AnaCanScan2.h"
#include <Diag.h>
#include <CanModuleUtils.h>

#ifdef _WIN32
#define DLLEXPORTFLAG __declspec(dllexport)
#else
#define DLLEXPORTFLAG  
#define WINAPI  
#endif

using namespace CanModule;
std::recursive_mutex anagateReconnectMutex;

/* static */ std::map<int, AnaCanScan2::ANAGATE_PORTDEF_t> AnaCanScan2::st_canHandleMap; // map handles to  {ports, ip}
/* static */ Log::LogComponentHandle AnaCanScan2::st_logItHandleAnagate = 0;
/* static */ std::map< std::string,bool > AnaCanScan2::st_reconnectInProgress_map;

/** global map of connection-objects: the map-key is the handle. Since handles are allocated by the OS
 * the keys are getting changed as well when we reconnect, so that we do not keep the stale keys(=handles) in
 * the map at all.
 * The map is used in the (static) InternalCallback code straightforward and speedy, because we can get to the object by just
 * looking up the map using the key(=the handle). The map manipulations are protected by serialization of reconnects
 * with the anagateReconnectMutex. At RO access in the callbacks the map is NOT protected because we do not want to
 * serialize the callbacks and because it is a RO map access, which is "like atomic". Well - it does work in multithreads.. ;-)
 */
std::map<AnaInt32, AnaCanScan2*> g_AnaCanScanObjectMap; // map handles to objects

#define MLOGANA2(LEVEL,THIS) LOG( Log::LEVEL, AnaCanScan2::st_logItHandleAnagate) << __FUNCTION__ << " " << " anagate bus= " << THIS->getBusName() << " "

AnaCanScan2::AnaCanScan2():
	m_canPortNumber(0),
	m_canIPAddress( (char *) std::string("192.168.1.2").c_str()),
	m_baudRate(0),
	m_idCanScanThread(0),
	m_canCloseDevice(false),
	m_busName(""),
	m_busParameters(""),
	m_UcanHandle(0),
	m_timeout ( 6000 ),
	m_busStopped( false ),
	m_gsig( NULL )
{
	m_statistics.setTimeSinceOpened();
	m_statistics.beginNewRun();
	m_failedSendCountdown = m_maxFailedSendCount;
	m_hCanReconnectionThread = new std::thread( &AnaCanScan2::m_CanReconnectionThread, this);
}

/**
 * CANCanDeviceConnectState , translate from counter
 * 1 = DISCONNECTED   :
 * 2 = CONNECTING :
 * 3 = CONNECTED
 * 4 = DISCONNECTING
 * 5 = NOT_INITIALIZED
 * b3...b27: unused
 *
 * into simple bitpattern (counter)
 * 0, 10, 11, 100, 101
 *
 */
uint32_t AnaCanScan2::getPortStatus(){
	AnaInt32 state = CANDeviceConnectState( m_UcanHandle );
	return( state | CANMODULE_STATUS_BP_ANAGATE );
}

/**
 * Shut down can scan thread
 */
AnaCanScan2::~AnaCanScan2()
{
	stopBus();
	MLOGANA2(DBG,this) << __FUNCTION__ <<" closed successfully";
}

/**
 * (virtual) forced implementation. Generally: do whatever shenanigans you need on the vendor API and fill in the portState accordingly, stay
 * close to the semantics of the enum.
 *
 * anagate specific implementation: obtain port status from the vendor, which is VERY sparse
 */
/* virtual */ void AnaCanScan2::fetchAndPublishCanPortState ()
{
	// we could also use the unified port status for this and strip off the implementation bits
	// but using can_get_state directly is less complex and has less dependencies

	unsigned int portState;
	AnaInt32 ana_state = CANDeviceConnectState( m_UcanHandle );

	// MLOGANA2(TRC,this) << __FUNCTION__ << " port_status_change ana_state= " << ana_state;

	// translate the ana_state ( AnaGateAPI.v2 from  2014-12-10 applies to 2.06-25.03.2021 CANDeviceConnectState )
	// into the standardized enum CanModule::CanModuleUtils::CanModule_bus_state
	// anagate does not give tx/rx error counts
	switch ( ana_state ){
	case ANAGATE_API_V2_DEVICE_CONNECTION_STATE::DISCONNECTED : {
		portState = CanModule::CanModule_bus_state::CAN_STATE_STOPPED;
		std::string msg = CanModule::translateCanBusStateToText((CanModule::CanModule_bus_state) portState );
		MLOGANA2(WRN, this) << msg << "tid=[" << std::this_thread::get_id() << "]";
		break;
	}
	case ANAGATE_API_V2_DEVICE_CONNECTION_STATE::CONNECTING : {
		portState = CanModule::CanModule_bus_state::CANMODULE_WARNING;
		std::string msg = CanModule::translateCanBusStateToText((CanModule::CanModule_bus_state) portState );
		MLOGANA2(WRN, this) << msg << "tid=[" << std::this_thread::get_id() << "]";
		break;
	}
	case ANAGATE_API_V2_DEVICE_CONNECTION_STATE::CONNECTED : {
		portState = CanModule::CanModule_bus_state::CANMODULE_OK;
		break;
	}
	case ANAGATE_API_V2_DEVICE_CONNECTION_STATE::DISCONNECTING : {
		portState = CanModule::CanModule_bus_state::CANMODULE_WARNING;
		std::string msg = CanModule::translateCanBusStateToText((CanModule::CanModule_bus_state) portState );
		MLOGANA2(WRN, this) << msg << "tid=[" << std::this_thread::get_id() << "]";
		break;
	}
	case ANAGATE_API_V2_DEVICE_CONNECTION_STATE::NOT_INITIALIZED : {
		portState = CanModule::CanModule_bus_state::CANMODULE_NOSTATE;
		break;
	}
	default: {
		portState = CanModule::CanModule_bus_state::CANMODULE_NOSTATE;
		std::string msg = CanModule::translateCanBusStateToText((CanModule::CanModule_bus_state) portState );
		MLOGANA2(WRN, this) << msg << "tid=[" << std::this_thread::get_id() << "]";
		break;
	}
	} // switch
	publishPortStatusChanged( portState );
}


void AnaCanScan2::stopBus ()
{
	if (!m_busStopped){
		MLOGANA2(TRC,this) << __FUNCTION__ << " stopping anagate m_busName= " <<  m_busName << " m_canPortNumber= " << m_canPortNumber;
		CANSetCallback( m_UcanHandle, 0);
		CANCloseDevice( m_UcanHandle);

		fetchAndPublishCanPortState();

		st_deleteCanHandleOfPortIp( m_canPortNumber, m_canIPAddress );
		m_eraseReceptionHandlerFromMap( m_UcanHandle );
	}
	m_busStopped = true; // we keep the object. we can re-open the bus
}


/* static */ void AnaCanScan2::st_setIpReconnectInProgress( std::string ip, bool flag ){
	// only called inside the locked mutex
	std::map< std::string, bool >::iterator it = AnaCanScan2::st_reconnectInProgress_map.find( ip );

	if ( flag ){
		// mark as in progress if not yet exists/marked
		if ( it == AnaCanScan2::st_reconnectInProgress_map.end() ) {
			AnaCanScan2::st_reconnectInProgress_map.insert ( std::pair< std::string, bool >( ip, true ) );
		}
	} else {
		// erase existing if still exists/marked
		if ( it != AnaCanScan2::st_reconnectInProgress_map.end() ) {
			AnaCanScan2::st_reconnectInProgress_map.erase( it );
		}
	}
}

/**
 * checks if a reconnection is already in progress for the bridge at that IP. Several ports can (and will) fail
 * at the same time, but the bridge should be reset only once, and not again for each port.
 */
/* static */ bool AnaCanScan2::st_isIpReconnectInProgress( std::string ip ){
	// only called inside the locked mutex
	std::map< std::string, bool >::iterator it = AnaCanScan2::st_reconnectInProgress_map.find( ip );
	if ( it == AnaCanScan2::st_reconnectInProgress_map.end() )
		return( false );
	else
		return( true );
}


/**
 * creates and returns anagate can access object
 */
extern "C" DLLEXPORTFLAG CCanAccess *getCanBusAccess()
{
	CCanAccess *canAccess = new AnaCanScan2;
	return canAccess;
}


/**
 * call back to catch incoming CAN messages for reading. It is called on a handle, which is internally
 * registered and managed by the anagate API. If that handle changes, the callback has to be unregistered before I guess.
 */
void WINAPI InternalCallback(AnaUInt32 nIdentifier, const char * pcBuffer, AnaInt32 nBufferLen, AnaInt32 nFlags, AnaInt32 hHandle, AnaInt32 nSeconds, AnaInt32 nMicroseconds)
{
	CanMessage canMsgCopy;
	if (nFlags == 2) return;
	canMsgCopy.c_id = nIdentifier;
	canMsgCopy.c_dlc = nBufferLen;
	canMsgCopy.c_ff = nFlags;
	for (int i = 0; i < nBufferLen; i++)
		canMsgCopy.c_data[i] = pcBuffer[i];

	canMsgCopy.c_time = CanModule::convertTimepointToTimeval(currentTimeTimeval());

	// more hardcode debugging, leave it in
	//cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
	//		<< " anagate message reception hHandle= " << hHandle
	//		<< " nIdentifier= " << nIdentifier
	//		<< endl;
	// MLOGANA2(TRC, g_AnaCanScanPointerMap[hHandle] ) << "read(): " << canMessageToString(canMsgCopy);

	LOG( Log::TRC ) << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
			<< " CanModule anagate " << CanModule::canMessageToString(canMsgCopy);

	g_AnaCanScanObjectMap[hHandle]->callbackOnRecieve(canMsgCopy);
	g_AnaCanScanObjectMap[hHandle]->statisticsOnRecieve( nBufferLen );
}


/**
 *  update statistics API
 */
void AnaCanScan2::statisticsOnRecieve(int bytes)
{
	m_statistics.setTimeSinceReceived();
	m_statistics.onReceive( bytes );
}

/**
 *  callback API: sends a boost signal with payload
 */
void AnaCanScan2::callbackOnRecieve( CanMessage& msg )
{
	canMessageCame( msg );
	if ( m_reconnectCondition == ReconnectAutoCondition::timeoutOnReception ){
		resetTimeoutOnReception();
	}

	static int calls = 100;
	if ( !calls-- ) {
		fetchAndPublishCanPortState();
		calls = 100;
	}
}

/**
 * Method that initialises a CAN bus channel for anagate. All following methods called on the same object will be using this initialized channel.
 *
 * @param name 3 parameters separated by ":" like "n0:n1:n2"
 * 		n0 = "an" for anagate
 * 		n1 = port number on the module, 0=A, 1=B, etc etc
 * 		n2 = ip number
 * 		ex.: "an:can1:137.138.12.99" speaks to port B (=1) on anagate module at the ip
 * 		ex.: "an:1:137.138.12.99" works as well
 *
 *
 * @param parameters up to 5 parameters separated by whitespaces : "p0 p1 p2 p3 p4" in THAT order, positive integers
 * 				* "Unspecified" (or empty): using defaults = "125000 0 0 0 0" // all params missing
 * 				* p0: bitrate: 10000, 20000, 50000, 62000, 100000, 125000, 250000, 500000, 800000, 1000000 bit/s
 * 				* p1: operatingMode: 0=default mode, values 1 (loop back) and 2 (listen) are not supported by CanModule
 * 				* p2: termination: 0=not terminated (default), 1=terminated (120 Ohm for CAN bus)
 * 				* p3: highSpeed: 0=deactivated (default), 1=activated. If activated, confirmation and filtering of CAN traffic are switched off
 * 				      for baud rates > 125000 this flag is needed, and it is set automatically for you if you forgot it (WRN)
 * 				* p4: TimeStamp: 0=deactivated (default), 1=activated. If activated, a timestamp is added to the CAN frame. Not all modules support this.
 *				  i.e. "250000 0 1 0 0"
 * 				(see anagate manual for more details)
 *
 * @return
 * 0: new bus, OK, add it please
 * 1: existing bus, OK, do not add
 * -1: error
 */
int AnaCanScan2::createBus(const std::string name, const std::string parameters)
{	
	m_busName = name;
	m_busParameters = parameters;
	m_gsig = GlobalErrorSignaler::getInstance();

	/** LogIt: initialize shared lib. The logging levels for the component logging is kept, we are talking still to
	 * the same master object "from the exe". We get the logIt ptr
	 * acquired down from the superclass, which keeps it as a static, and being itself a shared lib. we are inside
	 * another shared lib - 2nd stage - so we need to Dll initialize as well. Since we have many CAN ports we just acquire
	 * the handler to go with the logIt object and keep that as a static. we do not do per-port component logging.
	 * we do, however, stamp the logging messages specifically for each vendor using the macro.
	 */
	LogItInstance *logIt = getLogItInstance(); // CCanAccess inherited method
	// LogItInstance *logIt = CCanAccess::st_getLogItInstance();
	Log::LogComponentHandle myHandle;
	if ( logIt != NULL ){
		if (!Log::initializeDllLogging( logIt )){
			std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
					<< " could not DLL init remote LogIt instance " << std::endl;
		}
		logIt->getComponentHandle( CanModule::LogItComponentName, myHandle );
		LOG(Log::INF, myHandle ) << CanModule::LogItComponentName << " Dll logging initialized OK";
		AnaCanScan2::st_logItHandleAnagate = myHandle;
	} else {
		std::stringstream msg;
		msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " LogIt instance is NULL";
		std::cout << msg.str() << std::endl;
		m_gsig->fireSignal( 002, msg.str().c_str() );
	}

	/**
	 * lets get clear about the Logit components and their levels at this point
	 */
	std::map<Log::LogComponentHandle, std::string> log_comp_map = Log::getComponentLogsList();
	std::map<Log::LogComponentHandle, std::string>::iterator it;
	LOG(Log::TRC, myHandle ) << " *** Lnb of LogIt components= " << log_comp_map.size() << std::endl;
	for ( it = log_comp_map.begin(); it != log_comp_map.end(); it++ )
	{
		Log::LOG_LEVEL level;
		Log::getComponentLogLevel( it->first, level);
		LOG(Log::TRC, myHandle )  << " *** " << " LogIt component " << it->second << " level= " << level;
	}

	// board configuration
	int returnCode = m_configureCanBoard( name, parameters );
	if ( returnCode < 0 ) {
		m_signalErrorMessage( -1, "createBus: configureCanBoard: problem configuring. ip= " + m_canIPAddress +
				" name= " + name + " parameters= " + parameters );
		return -1;
	}
	MLOGANA2(DBG,this) << " OK, Bus created with name= " << name << " parameters= " << parameters;
	m_busStopped = false;
	fetchAndPublishCanPortState();

	return 0;
}


/**
 * decode the name, parameter and return the port of the configured module
 *  i.e. name="an:0:128.141.159.194"
 *  i.e. parameters= "p0 p1 p2 p3 p4 p5 p6" with i.e. "125000 0 1 0 0 0 6000"
 *  p0=baudrate
 *  p1=	operation mode
 *  p2 = termination
 *  p3 = high speed flag
 *  p4 = time stamp
 *  p5 = sync mode
 *  p6 = timeout/ms
 */
int AnaCanScan2::m_configureCanBoard(const std::string name,const std::string parameters)
{
	MLOGANA2(DBG, this) << "(user supplied) name= " << name << " parameters= " << parameters;
	std::vector< std::string > stringVector;
	stringVector = parseNameAndParameters(name, parameters);

	// we should decode 3 elements from this:0="an" for anagate library, 1=can port, 2=ip number
	// like  "an:0:128.141.159.194"
	if ( stringVector.size() != 3 ){
		MLOGANA2(ERR, this) << " need exactly 3 elements delimited by \":\" like \"an:0:128.141.159.194\", got "
				<< stringVector.size();
		for ( unsigned i = 0; i < stringVector.size(); i++ ){
			MLOGANA2(ERR, this) << " stringVector[" << i << "]= " << stringVector[ i ];
		}
		m_signalErrorMessage( -1, "configureCanBoard: need exactly 3 parameters i.e.  `an:0:128.141.159.194` . ip= " + m_canIPAddress +
				" name= " + name + " parameters= " + parameters );
		return(-1);
	} else {
		for ( unsigned i = 0; i < stringVector.size(); i++ ){
			MLOGANA2(TRC, this) << "(cleaned up) stringVector[" << i << "]= " << stringVector[ i ];
		}
		m_canPortNumber = atoi(stringVector[1].c_str());
		m_canIPAddress = (char *) stringVector[2].c_str();
	}
	MLOGANA2(TRC, this) << "(cleaned up) canPortNumber= " << m_canPortNumber << " ip= " << m_canIPAddress;

	// handle up to 7 parameter, assume defaults if needed
	m_baudRate = 125000L;

	if (strcmp(parameters.c_str(), "Unspecified") != 0) {
		MLOGANA2(TRC, this) << "m_CanParameters.m_iNumberOfDetectedParameters " << m_CanParameters.m_iNumberOfDetectedParameters;
		if ( m_CanParameters.m_iNumberOfDetectedParameters >= 1 )	{
			m_baudRate = m_CanParameters.m_lBaudRate; // just for the statistics

			if (( m_CanParameters.m_lBaudRate > 125000L ) && ( ! m_CanParameters.m_iHighSpeed )){
				MLOGANA2(WRN, this) << "baud rate is above 125000bits/s, and the high speed flag is not set, but needed. Setting high speed flag for you. To avoid this warning set the high speed flag properly in your configuration";
				m_CanParameters.m_iHighSpeed = 1;
			}

			if ( m_CanParameters.m_iNumberOfDetectedParameters >= 7 )	{
				m_timeout = m_CanParameters.m_iTimeout; // here we set it: CANT-44
				MLOGANA2(TRC, this) << "anagate: picked up from 7th parameter: m_timeout= " << m_timeout;
			}

			// any other parameters are already set, either to 0 by init,
			// or by decoding. They are always used.
		} else {
			MLOGANA2(ERR, this) << "Error while parsing parameters: this syntax is incorrect: [" << parameters << "]";
			MLOGANA2(ERR, this) << "you need up to 7 numbers separated by whitespaces, i.e. \"125000 0 1 0 0 0 6000\" \"p0 p1 p2 p3 p4 p5 p6\"";
			MLOGANA2(ERR, this) << "  p0 = baud rate, 125000 or whatever the module supports";
			MLOGANA2(ERR, this) << "  p1 = operation mode";
			MLOGANA2(ERR, this) << "  p2 = termination";
			MLOGANA2(ERR, this) << "  p3 = high speed";
			MLOGANA2(ERR, this) << "  p4 = time stamp";
			MLOGANA2(ERR, this) << "  p5 = sync mode";
			MLOGANA2(ERR, this) << "  p6 = timeout/ms";
			return -1;
		}
	} else	{
		// Unspecified: we use defaults "125000 0 1 0 0 0 6000"
		MLOGANA2(INF, this) << "Unspecified parameters, default values (termination=1) will be used.";
		m_CanParameters.m_lBaudRate = 125000;
		m_CanParameters.m_iOperationMode = 0;
		m_CanParameters.m_iTermination = 1 ;// ENS-26903: default is terminated
		m_CanParameters.m_iHighSpeed = 0;
		m_CanParameters.m_iTimeStamp = 0;
		m_CanParameters.m_iSyncMode = 0;
		m_CanParameters.m_iTimeout = 6000; // CANT-44: can be set
	}
	MLOGANA2(TRC, this) << __FUNCTION__ << " m_lBaudRate= " << m_CanParameters.m_lBaudRate;
	MLOGANA2(TRC, this) << __FUNCTION__ << " m_iOperationMode= " << m_CanParameters.m_iOperationMode;
	MLOGANA2(TRC, this) << __FUNCTION__ << " m_iTermination= " << m_CanParameters.m_iTermination;
	MLOGANA2(TRC, this) << __FUNCTION__ << " m_iHighSpeed= " << m_CanParameters.m_iHighSpeed;
	MLOGANA2(TRC, this) << __FUNCTION__ << " m_iTimeStamp= " << m_CanParameters.m_iTimeStamp;
	MLOGANA2(TRC, this) << __FUNCTION__ << " m_iSyncMode= " << m_CanParameters.m_iSyncMode;
	MLOGANA2(TRC, this) << __FUNCTION__ << " m_iTimeout= " << m_CanParameters.m_iTimeout;
	// no hw interaction up to this point

	return m_openCanPort(); // hw interaction
}

/**
 * add a can handle into the map if it does not yet exist
 */
/* static */ void AnaCanScan2::st_addCanHandleOfPortIp(AnaInt32 handle, int port, std::string ip) {
	std::map<AnaInt32, ANAGATE_PORTDEF_t>::iterator it = st_canHandleMap.find( handle );
	if ( it != st_canHandleMap.end() ){
		LOG(Log::WRN, AnaCanScan2::st_logItHandleAnagate) << __FUNCTION__
				<< " " << handle << " is opened already on "
				<< " port= " << port << " ip= " << ip;
	} else {
		ANAGATE_PORTDEF_t pd;
		pd.ip = ip;
		pd.port = port;
		st_canHandleMap.insert(std::pair<AnaInt32, ANAGATE_PORTDEF_t>(handle, pd));
	}
}

/**
 * find and erase a map entry handle->{port, ip}
 * todo: mutex protect, work on copy
 */
/* static */ void AnaCanScan2::st_deleteCanHandleOfPortIp(int port, std::string ip){
	ANAGATE_PORTDEF_t pd;
	pd.ip = ip;
	pd.port = port;
	std::map<AnaInt32, ANAGATE_PORTDEF_t>::iterator it = st_canHandleMap.begin();
	int handle = -1;
	while ( it != st_canHandleMap.end() ){
		if (( it->second.port == port) && ( it->second.ip == ip)){
			handle = it->first;
		}
		it++;
	}
	if ( handle > -1 ) {
		st_canHandleMap.erase( handle );
	} else {
		LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate) << __FUNCTION__
				<< " could not erase handle= " << handle
				<< " for port= " << port << " ip= " << ip;
	}
}

/**
 * find the handle from port, ip
 * we have to search through the whole map to know
 */
/* static */ AnaInt32 AnaCanScan2::st_getCanHandleOfPortIp(int port, std::string ip) {

	LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate) << __FUNCTION__
			<< " port= " << port << " ip= " << ip;
	std::map<AnaInt32, ANAGATE_PORTDEF_t>::iterator it = st_canHandleMap.begin();
	while ( it != st_canHandleMap.end() ){
		if (( it->second.port == port) && ( it->second.ip == ip)){
			return( it->first );
		}
		it++;
	}
	return(-1);
}

/**
 * Obtains an Anagate canport and opens it.
 *  The name of the port and parameters should have been specified by preceding call to configureCanboard()
 *  @returns less than zero in case of error, otherwise success
 *
 * communicate with the hardware using the CanOpen interface:
 * open anagate port, attach reply handler.
 * No message queuing. CANSetMaxSizePerQueue not called==default==0
 */
int AnaCanScan2::m_openCanPort()
{
	AnaInt32 anaCallReturn;
	AnaInt32 canModuleHandle;

	/**
	 * check if that anagate bridge (ip) is already initialized, if not we create it.
	 */
	canModuleHandle = st_getCanHandleOfPortIp(m_canPortNumber, m_canIPAddress );
	if ( canModuleHandle < 0 ){
		MLOGANA2(DBG, this) << "calling CANOpenDevice with port= " << m_canPortNumber << " ip= " << m_canIPAddress
				<< " canModuleHandle= " << canModuleHandle
				<< " m_timeout= " << m_timeout;
		anaCallReturn = CANOpenDevice(&canModuleHandle, FALSE, TRUE, m_canPortNumber, m_canIPAddress.c_str(), m_timeout);
		if (anaCallReturn != ANA_ERR_NONE) {
			std::stringstream os;
			os << " openCanPort: Error for port= " << m_canPortNumber << " ip= " << m_canIPAddress;
			m_signalErrorMessage( anaCallReturn, os.str().c_str() ); // most likely the handler is not yet connected and we don't have a canbus object
			MLOGANA2(ERR,this) << "Error in CANOpenDevice, return code = [0x" << std::hex << anaCallReturn << std::dec << "]";
			return -1;
		}
	} else {
		MLOGANA2(DBG, this) << "port= " << m_canPortNumber << " ip= " << m_canIPAddress
				<< " is open already, got handle= "
				<< canModuleHandle;
	}

	// initialize CAN interface
	MLOGANA2(TRC,this) << "calling CANSetGlobals with m_lBaudRate= "
			<< m_CanParameters.m_lBaudRate
			<< " m_iOperationMode= " << m_CanParameters.m_iOperationMode
			<< " m_iTermination= " << m_CanParameters.m_iTermination
			<< " m_iHighSpeed= " << m_CanParameters.m_iHighSpeed
			<< " m_iTimeStamp= " << m_CanParameters.m_iTimeStamp
			<< " handle= " << canModuleHandle;
	anaCallReturn = CANSetGlobals(canModuleHandle, m_CanParameters.m_lBaudRate, m_CanParameters.m_iOperationMode,
			m_CanParameters.m_iTermination, m_CanParameters.m_iHighSpeed, m_CanParameters.m_iTimeStamp);
	switch ( anaCallReturn ){
	case 0:{ break; }
	case 0x30000: {
		MLOGANA2(ERR,this) << "Connection to TCP/IP partner can't be established or is disconnected. Lost TCP/IP: 0x" << std::hex << anaCallReturn << std::dec;
		m_signalErrorMessage( anaCallReturn, " openCanPort: CANSetGlobals: Connection to TCP/IP partner can't be established or is disconnected. Lost TCP/IP." );
		return -1;
	}
	case 0x40000: {
		MLOGANA2(ERR,this) << "No  answer  was  received  from	TCP/IP partner within the defined timeout. Lost TCP/IP: 0x" << std::hex << anaCallReturn << std::dec;
		m_signalErrorMessage( anaCallReturn, " openCanPort: CANSetGlobals: No  answer  was  received  from	TCP/IP partner within the defined timeout. Lost TCP/IP." );
		return -1;
	}
	case 0x900000: {
		MLOGANA2(ERR,this) << "Invalid device handle. Lost TCP/IP: 0x" << std::hex << anaCallReturn << std::dec;
		m_signalErrorMessage( anaCallReturn, " openCanPort: CANSetGlobals: Invalid device handle. Lost TCP/IP." );
		return -1;
	}
	default : {
		MLOGANA2(ERR,this) << "Other Error in CANSetGlobals: 0x" << std::hex << anaCallReturn << std::dec;
		m_signalErrorMessage( anaCallReturn, " openCanPort: CANSetGlobals: Other Error in CANSetGlobals." );
		return -1;
	}
	}
	// set handler for managing incoming messages
	MLOGANA2(TRC,this) << "calling CANSetCallbackEx handle= " << canModuleHandle;
	anaCallReturn = CANSetCallbackEx(canModuleHandle, InternalCallback);
	if (anaCallReturn != ANA_ERR_NONE) {
		MLOGANA2(ERR,this) << "Error in CANSetCallbackEx, return code = [" << anaCallReturn << "]";
		m_signalErrorMessage( anaCallReturn, " openCanPort: CANSetCallbackEx: Error in CANSetCallbackEx." );
		return -1;
	} else {
		MLOGANA2(TRC,this) << "OK CANSetCallbackEx, return code = [" << anaCallReturn << "]";
	}

	// bookkeeping in maps for {port, ip} and object pointers
	st_addCanHandleOfPortIp( canModuleHandle, m_canPortNumber, m_canIPAddress );
	g_AnaCanScanObjectMap[canModuleHandle] = this; // gets added
	m_UcanHandle = canModuleHandle;

	// keep some hardcore debugging
	// AnaCanScan2::showAnaCanScanObjectMap();

	// initial port state
	fetchAndPublishCanPortState();

	return 0;
}


/**
 * wrapper to send an error code plus message down the signal. Can send any code with any text.
 * use ana_canGetErrorText for ana error codes as well
 *
 * boost::signals2::signal<void (const int,const char *,timeval &) > canMessageError;
 */
void AnaCanScan2::m_signalErrorMessage( int code, std::string msg )
{
	timeval ftTimeStamp;
	auto now = std::chrono::high_resolution_clock::now();
	auto nMicrosecs =
			std::chrono::duration_cast<std::chrono::microseconds>( now.time_since_epoch());
	ftTimeStamp.tv_sec = nMicrosecs.count() / 1000000L;
	ftTimeStamp.tv_usec = (nMicrosecs.count() % 1000000L) ;
	msg = getBusName() + msg; // force prefix bus name in all erors
	canMessageError( code, msg.c_str(), ftTimeStamp);
}

/**
 * send a CAN message frame (8 byte) for anagate
 * Method that sends a message through the can bus channel. If the method createBus was not called before this, sendMessage will fail, as there is no
 * can bus channel to send a message trough.
 * @param cobID: Identifier that will be used for the message.
 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
 * @param message: Message to be sent trough the can bus.
 * @param rtr: is the message a remote transmission request?
 * @return Was the sending process successful?
 */
bool AnaCanScan2::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{

	if ( m_canCloseDevice || m_busStopped ){
		MLOGANA2(WRN,this) << __FUNCTION__ << " bus is closed, skipping [ closed= " << m_canCloseDevice << " stopped= " << m_busStopped << "]";
		m_signalErrorMessage( -1, "sendMessage: the bus is closed or stopped, skipping send. ip= " + m_canIPAddress);
		return( false );
	}

	// /* static */ std::string AnaCanScan2::canMessageToString(CanMessage &f)
	// MLOGANA2(DBG,this) << "Sending message: [" << ( message == 0  ? "" : (const char *) message) << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";

	MLOGANA2(DBG,this) << __FUNCTION__ << " Sending message: [" << CanModule::canMessage2ToString(cobID, len, message, rtr) << "]";
	AnaInt32 anaCallReturn = 0;
	unsigned char *messageToBeSent[8];
	AnaInt32 flags = 0x0;
	if (rtr) {
		flags = 2; // Bit 1: If set, the telegram is marked as remote frame.
	}
	int  messageLengthToBeProcessed;

	//If there is more than 8 characters to process, we process 8 of them in this iteration of the loop
	if (len > 8) {
		messageLengthToBeProcessed = 8;
		MLOGANA2( ERR, this) << __FUNCTION__ << " The length is more than 8 bytes, ignoring overhead " << len;
		m_signalErrorMessage( -2, "sendMessage: the CAN message length to send exceeds 8 bytes, ignoring overhead. ip= " + m_canIPAddress);
	} else {
		messageLengthToBeProcessed = len;
	}

	/**
	 * before sending the message we check if the reception timeout has not been overrun. Because this
	 * would entail some reconnection action, and in that case we skip the send on that bus.
	 * we invoke the thread only if the timeout has already occurred, and so we do not need to check the timeout in
	 * the thread anymore.
	 */
	if ( getReconnectCondition() == CanModule::ReconnectAutoCondition::timeoutOnReception && hasTimeoutOnReception()) {
		MLOGANA2(WRN, this) << __FUNCTION__ << " m_canPortNumber= " << m_canPortNumber << " skipping send, detected "
				<< reconnectConditionString(m_reconnectCondition);

		//send a reconnection thread trigger
		MLOGANA2(DBG, this) << __FUNCTION__ << " trigger reconnection thread to check reception timeout " << getBusName();
		triggerReconnectionThread();

	} else {

		/**
		 *  we can send now
		 */
		memcpy(messageToBeSent, message, messageLengthToBeProcessed);
		MLOGANA2(TRC, this) << __FUNCTION__<< " m_canPortNumber= " << m_canPortNumber
				<< " cobID= " << cobID
				<< " length = " << messageLengthToBeProcessed
				<< " flags= " << flags << " m_UcanHandle= " << m_UcanHandle;
		anaCallReturn = CANWrite(m_UcanHandle, cobID, reinterpret_cast<char*>(messageToBeSent), messageLengthToBeProcessed, flags);
		if (anaCallReturn != ANA_ERR_NONE) {
			m_signalErrorMessage( anaCallReturn, "sendMessage: CANWrite: problem writing message. ip= " + m_canIPAddress);
		}
	}

	if (anaCallReturn != ANA_ERR_NONE) {
		MLOGANA2(ERR, this) << __FUNCTION__ << " There was a problem when sending/receiving timeout with CANWrite or a reconnect condition: 0x"
				<< std::hex << anaCallReturn << std::dec
				<< " ip= " << m_canIPAddress;
		m_canCloseDevice = false;
		decreaseSendFailedCountdown();

		//send a reconnection thread trigger
		MLOGANA2(DBG, this) << __FUNCTION__ << " trigger reconnection thread since a send failed " << getBusName();
		triggerReconnectionThread();
	} else {
		m_statistics.onTransmit(messageLengthToBeProcessed);
		m_statistics.setTimeSinceTransmitted();
	}
	// port state update, decrease load on the board a bit
	static int calls = 5;
	if ( !calls-- ) {
		fetchAndPublishCanPortState();
		calls = 5;
	}
	return ( true );
}

/**
 * display the handle->object map, and using the object, show more details
 */
/* static */ void AnaCanScan2::st_showAnaCanScanObjectMap(){
	uint32_t size = g_AnaCanScanObjectMap.size();
	LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate  ) << __FUNCTION__ << " RECEIVE obj. map size= " << size << " : ";
	for (std::map<AnaInt32, AnaCanScan2*>::iterator it=g_AnaCanScanObjectMap.begin(); it!=g_AnaCanScanObjectMap.end(); it++){
		LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate ) << __FUNCTION__ << " obj. map "
				<< " key= " << it->first
				<< " ip= " << it->second->m_ipAddress()
				<< " CAN port= " << it->second->m_CanPortNumber()
				<< " handle= " << it->second->m_handle()
				<< " ptr= 0x" << std::hex << (unsigned long) (it->second) << std::dec; // naja, just to see it tis != NULL

	}
}


/**
 * only reconnect this object's port and make sure the callback map entry is updated as well. deal with the handler
 */
AnaInt32 AnaCanScan2::m_reconnectThisPort(){
	// a sending on one port failed miserably, and we reset just that port.
	AnaInt32 anaCallReturn0 = m_reconnect();
	while ( anaCallReturn0 < 0 ){
		MLOGANA2(WRN, this) << __FUNCTION__<< " failed to reconnect CAN port " << m_canPortNumber
				<< " ip= " << m_canIPAddress << ",  try again";

		std::stringstream os;
		os << __FUNCTION__ << " failed to reconnect CAN port " << m_canPortNumber
				<< " ip= " << m_canIPAddress << ",  try again";

		m_signalErrorMessage( anaCallReturn0, os.str().c_str() );
		CanModule::ms_sleep( 5000 );
		anaCallReturn0 = m_reconnect();
	}
	st_showAnaCanScanObjectMap();
	anaCallReturn0 = m_connectReceptionHandler();
	LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate ) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
			<< " reconnect reception handler for port= " << m_canPortNumber
			<< " ip= " << m_canIPAddress
			<< " looking good= " << anaCallReturn0;

	// erase stale map entry for the old reception handler
	std::map<AnaInt32, AnaCanScan2*> lmap = g_AnaCanScanObjectMap;
	for (std::map<AnaInt32, AnaCanScan2*>::iterator it=lmap.begin(); it!=lmap.end(); it++){
		/**
		 * delete the map elements where the key is different from the handle, since we reassigned handles
		 * in the living objects like:
		 * 	g_AnaCanScanPointerMap[ m_UcanHandle ] = this;
		 */
		if ( it->first != it->second->m_handle()){
			LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
					<< " erasing stale handler " << it->first
					<< " for object handle= " << it->second->m_handle() << " from obj. map";
			g_AnaCanScanObjectMap.erase( it->first );
		}
	}

	// port state update
	fetchAndPublishCanPortState();

	return( anaCallReturn0 );
}



/**
 * reconnects all opened ports of a given ip address=one module. We have one object per CAN port,
 * so i.e. for an anagate duo (ports A and B) we have two objects with the same IP but
 * with port 0 and 1.So we will have to scan over all objects there because we do not know
 * how many CANports a given anagate bridge has. We also allow globally only one ongoing reconnect
 * to one ip at a time. Theoretically we could have a separate reconnection thread per ip, but that
 * looks like overcomplicating the code and the issue. If several anagate modules fail
 * they will all reconnect one after the other. That is OK since if all of these modules
 * loose power and reboot at the same moment they will also be ready at roughly the same moment
 * for reconnect. That means that we have to wait for the first ip to reconnect for a while
 * but then all other ips will reconnect rather quickly in the most likely case.
 * returns:
 * 0 = OK
 * -1 = cant open / reconnect CAN ports
 * +1 = ignore, since another thread is doing the reconnect already
 */
/* static */ AnaInt32 AnaCanScan2::st_reconnectAllPorts( std::string ip ){

	// protect against several calls on the same ip, recursive_mutex needed
	{
		anagateReconnectMutex.lock();
		if ( AnaCanScan2::st_isIpReconnectInProgress( ip ) ) {
			LOG(Log::WRN, AnaCanScan2::st_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
					<< " is already in progress, skipping.";
			CanModule::ms_sleep( 10000 );
			anagateReconnectMutex.unlock();
			return(1);
		}
		AnaCanScan2::st_setIpReconnectInProgress( ip, true );
		LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
				<< " is now in progress.";
		anagateReconnectMutex.unlock();
	}

	int ret = 0;
	AnaInt32 anaRet = 0;

	// re-open the can port and get a new handle, but do NOT yet put the new object (this, again) into
	// the global map. Keep the reception disconnected still.
	int nbCANportsOnModule = 0;
	bool reconnectFailed = false;
	// show the old handle on that ip
	for (std::map<AnaInt32, AnaCanScan2*>::iterator it=g_AnaCanScanObjectMap.begin(); it!=g_AnaCanScanObjectMap.end(); it++){
		if ( ip == it->second->m_ipAddress() ){
			LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate ) << __FUNCTION__
					<< " key= " << it->first
					<< " found ip= " << ip
					<< " for CAN port= " << it->second->m_CanPortNumber()
					<< " reconnecting...";

			ret = it->second->m_reconnect();
			if ( ret ){
				LOG(Log::WRN, AnaCanScan2::st_logItHandleAnagate) << __FUNCTION__
						<< " key= " << it->first
						<< " found ip= " << ip
						<< " for CAN port= " << it->second->m_CanPortNumber()
						<< " failed to reconnect";

				std::stringstream os;
				os << __FUNCTION__
						<< " key= " << it->first
						<< " found ip= " << ip
						<< " for CAN port= " << it->second->m_CanPortNumber()
						<< " failed to reconnect";
				it->second->m_signalErrorMessage( ret, os.str().c_str() );
				reconnectFailed = true;
			}
			nbCANportsOnModule++;
		}
	}
	LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate) << " CAN bridge at ip= " << ip << " uses  nbCANportsOnModule= " << nbCANportsOnModule;

	if ( reconnectFailed ) {
		LOG(Log::WRN, AnaCanScan2::st_logItHandleAnagate ) << " Problem reconnecting CAN ports for ip= " << ip
				<< " last ret= " << ret << ". Just abandoning and trying again in 10 secs, module might not be ready yet.";

		CanModule::ms_sleep( 10000 );

		AnaCanScan2::st_setIpReconnectInProgress( ip, false );
		LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
				<< " cancel.";
		return(-1);
	}

	// register the new handler with the obj. map and connect the reception handler. Cleanup the stale handlers
	// use a local copy of the map, in order not to change the map we are iterating on
	std::map<AnaInt32, AnaCanScan2*> lmap = g_AnaCanScanObjectMap;
	LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
			<< " receive handler map map before reconnect for ip= " << ip;
	AnaCanScan2::st_showAnaCanScanObjectMap();

	for (std::map<AnaInt32, AnaCanScan2*>::iterator it=lmap.begin(); it!=lmap.end(); it++){
		if ( ip == it->second->m_ipAddress() ){
			anaRet = it->second->m_connectReceptionHandler();
			if ( anaRet != ANA_ERR_NONE ){
				LOG(Log::ERR, AnaCanScan2::st_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
						<< " failed to reconnect reception handler for ip= " << ip
						<< " handle= " << it->second->m_handle()
						<< " port= " << it->second->m_CanPortNumber()
						<< " anaRet= " << anaRet;
			} else {
				LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate ) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
						<< " reconnect reception handler for ip= " << ip
						<< " handle= " << it->second->m_handle()
						<< " looking good= " << anaRet;
			}

			/**
			 * delete the map elements where the key is different from the handle, since we reassigned handles
			 * in the living objects like:
			 * 	g_AnaCanScanPointerMap[ m_UcanHandle ] = this;
			 */
			if ( it->first != it->second->m_handle()){
				LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
						<< " erasing stale handler " << it->first
						<< " for object handle= " << it->second->m_handle() << " from obj. map";
				g_AnaCanScanObjectMap.erase( it->first );
			}
			AnaCanScan2::st_setIpReconnectInProgress( ip, false ); // all done, may fail another time
			LOG(Log::INF, AnaCanScan2::st_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
					<< " is done and OK.";
		}
	}
	//LOG(Log::TRC, AnaCanScan2::st_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
	//		<< " receive handler map after reconnect for ip= " << ip;
	AnaCanScan2::st_showAnaCanScanObjectMap();

	// be easy on the switch
	CanModule::ms_sleep( 100 );
	return( anaRet );
}

/**
 * connect the reception handler and only AFTER that register the handle
 * in the global obj. map, since reception is asynchron.
 * Finally, in the caller, the stale handle is deleted from the map.
 */
AnaInt32 AnaCanScan2::m_connectReceptionHandler(){
	AnaInt32 anaCallReturn;

	// this is needed otherwise the bridge hangs in a bad state
	anaCallReturn = CANSetGlobals(m_UcanHandle, m_CanParameters.m_lBaudRate, m_CanParameters.m_iOperationMode,
			m_CanParameters.m_iTermination, m_CanParameters.m_iHighSpeed, m_CanParameters.m_iTimeStamp);
	if ( anaCallReturn != ANA_ERR_NONE){
		std::stringstream os;
		os << __FUNCTION__ 	<< " CANSetGlobals returned an error. ip= " << m_canIPAddress;
		m_signalErrorMessage( anaCallReturn, os.str().c_str() );
	}
	switch ( anaCallReturn ){
	case 0:{ break; }
	case 0x30000: {
		MLOGANA2(ERR,this) << "Connection to TCP/IP partner can't be established or is disconnected. Lost TCP/IP: 0x" << std::hex << anaCallReturn << std::dec;
		return -1;
	}
	case 0x40000: {
		MLOGANA2(ERR,this) << "No  answer  was  received  from	TCP/IP partner within the defined timeout. Lost TCP/IP: 0x" << std::hex << anaCallReturn << std::dec;
		return -1;
	}
	case 0x900000: {
		MLOGANA2(ERR,this) << "Invalid device handle. Lost TCP/IP: 0x" << std::hex << anaCallReturn << std::dec;
		return -1;
	}
	default : {
		MLOGANA2(ERR,this) << "Other Error in CANSetGlobals: 0x" << std::hex << anaCallReturn << std::dec;
		return -1;
	}
	}

	MLOGANA2(WRN,this) << "connecting RECEIVE canModuleHandle= " << m_UcanHandle
			<< " ip= " << m_canIPAddress;
	anaCallReturn = CANSetCallbackEx(m_UcanHandle, InternalCallback);
	if (anaCallReturn != 0) {
		MLOGANA2(ERR,this) << "failed CANSetCallbackEx, return code = [" << anaCallReturn << "]"
				<< " canModuleHandle= " << m_UcanHandle
				<< " in a reconnect! Can't fix this, maybe hardware/firmware problem.";
		std::stringstream os;
		os << __FUNCTION__ 	<< " failed CANSetCallbackEx, return code = [" << anaCallReturn << "]"
				<< " canModuleHandle= " << m_UcanHandle
				<< " in a reconnect! Can't fix this, maybe hardware/firmware problem.";
		m_signalErrorMessage( anaCallReturn, os.str().c_str() );

		// that is very schlecht. This can be mitigated by using reconnection behavior for reception timeout.
	}
	MLOGANA2(DBG,this) << "adding RECEIVE handler, handle= " << m_UcanHandle
			<< " CAN port= " << m_canPortNumber
			<< " ip= " << m_canIPAddress << " to obj.map.";
	g_AnaCanScanObjectMap[ m_UcanHandle ] = this;
	// setCanHandleInUse( m_UcanHandle, true );

	MLOGANA2(DBG,this) << "RECEIVE handler looks good, handle= " << m_UcanHandle
			<< " CAN port= " << m_canPortNumber
			<< " ip= " << m_canIPAddress << " Congratulations.";

	// port state update
	fetchAndPublishCanPortState();

	return( anaCallReturn );
}

/**
 * todo: protect with mutex, work on copy
 */
void AnaCanScan2::m_eraseReceptionHandlerFromMap( AnaInt32 h ){
	std::map<AnaInt32, AnaCanScan2*>::iterator it = g_AnaCanScanObjectMap.find( h );
	if (it != g_AnaCanScanObjectMap.end()) {
		g_AnaCanScanObjectMap.erase ( it );
		m_busName = "nobus";
	} else {
		MLOGANA2(TRC,this) << " handler " << h << " not found in map, not erased";
	}
}


/**
 * we try to reconnect one port after a power loss, and we should do this for all ports
 * returns:
 * 0 = OK
 * -1 = could not CANOpenDevice device
 * -2 = could not connect callback CANSetCallbackEx
 * -3 = device was found open, tried to close but did not succeed (maybe you should just try sending again)
 */
int AnaCanScan2::m_reconnect(){
	AnaInt32 anaCallReturn;
	AnaInt32 canModuleHandle;

	// we are not too fast. sleep to wait for network and don't hammer the switch
	CanModule::ms_sleep( 100 );

	MLOGANA2(WRN, this) << "try to reconnect ip= " << m_canIPAddress
			<< " m_canPortNumber= " << m_canPortNumber
			<< " m_UcanHandle= " << m_UcanHandle;

	int state = 5; // re-open completely unless otherwise
	if ( m_UcanHandle > 0 ){
		m_canCloseDevice = false;
	} else {
		// todo: we should probably drop this, if the handle isn't valid anyway
		state = CANDeviceConnectState( m_UcanHandle );
	}
	MLOGANA2(TRC, this) << "CANDeviceConnectState: device connect state= 0x" << std::hex << state << std::dec;
	/**
	•
	1 = DISCONNECTED
	: The connection to the AnaGate is disconnected.
	•
	2 = CONNECTING
	: The connection is connecting.
	•
	3 = CONNECTED
	: The connection is established.
	•
	4 = DISCONNECTING
	: The connection is disonnecting.
	•
	5 = NOT_INITIALIZED
	: The network protocol is not successfully initialized.
	 */

	switch ( state ){
	case 2: {
		MLOGANA2(INF,this) << "device is in state CONNECTING, don't try to reconnect for now, skip.";
		break;
	}

	case 3: {
		MLOGANA2(INF,this) << "device is in state CONNECTED, don't try to reconnect, just skip.";
		break;
	}
	default:
	case 1:
	case 4:
	case 5:{
		if ( !m_canCloseDevice ) {
			anaCallReturn = CANCloseDevice( m_UcanHandle );
			MLOGANA2(WRN, this) << "closed device m_UcanHandle= " << m_UcanHandle
					<< " anaCallReturn= 0x" << std::hex << anaCallReturn << std::dec;
			if ( anaCallReturn != ANA_ERR_NONE ) {
				MLOGANA2(WRN, this) << "could not close device m_UcanHandle= " << m_UcanHandle
						<< " anaCallReturn= 0x" << std::hex << anaCallReturn << std::dec;
				// return(-3);
			}
			m_canCloseDevice = true;
			MLOGANA2(TRC, this) << "device is closed. stale handle= " << m_UcanHandle;
		} else {
			MLOGANA2(WRN, this) << "device is already closed, try reopen. stale handle= " << m_UcanHandle;
		}
		//setCanHandleInUse( m_UcanHandle, false );
		st_deleteCanHandleOfPortIp( m_canPortNumber, m_canIPAddress );

		MLOGANA2(TRC,this) << "try CANOpenDevice m_canPortNumber= " << m_canPortNumber
				<< " m_UcanHandle= " << m_UcanHandle
				<< " ip= " << m_canIPAddress << " timeout= " << m_timeout;
		anaCallReturn = CANOpenDevice(&canModuleHandle, FALSE, TRUE, m_canPortNumber, m_canIPAddress.c_str(), m_timeout);
		if (anaCallReturn != ANA_ERR_NONE) {
			MLOGANA2(ERR,this) << "CANOpenDevice failed: 0x" << std::hex << anaCallReturn << std::dec;
			m_canCloseDevice = false;

			std::stringstream os;
			os << __FUNCTION__ 	<< "CANOpenDevice failed: 0x" << std::hex << anaCallReturn << std::dec;
			m_signalErrorMessage( anaCallReturn, os.str().c_str() );
			return(-1);
		}
		MLOGANA2(DBG,this) << "CANOpenDevice m_canPortNumber= " << m_canPortNumber
				<< " canModuleHandle= " << canModuleHandle
				<< " ip= " << m_canIPAddress << " timeout= " << m_timeout << " reconnect for SEND looks good";

		st_addCanHandleOfPortIp( canModuleHandle, m_canPortNumber, m_canIPAddress );
		m_UcanHandle = canModuleHandle;
		m_canCloseDevice = false;
		// object in object map stays the same
		break;
	}
	}
	CanModule::ms_sleep( 100 );

	// port state update
	fetchAndPublishCanPortState();

	return( 0 ); // OK
}


bool AnaCanScan2::sendMessage(CanMessage *canm)
{
	return sendMessage(short(canm->c_id), canm->c_dlc, canm->c_data, canm->c_rtr);
}

/**
 * Method that sends a remote request trough the can bus channel.
 * If the method createBus was not called before this, sendMessage will fail, as there is no
 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
 * @param cobID: Identifier that will be used for the request.
 * @return Was the initialisation process successful?
 */
bool AnaCanScan2::sendRemoteRequest(short cobID)
{
	AnaInt32 anaCallReturn;
	AnaInt32 flags = 2;// Bit 1: If set, the telegram is marked as remote frame.

	anaCallReturn = CANWrite(m_UcanHandle,cobID,NULL, 0, flags);
	if (anaCallReturn != ANA_ERR_NONE) {
		MLOGANA2(ERR,this) << "There was a problem when sending a message with CANWrite";

		std::stringstream os;
		os << __FUNCTION__ 	<< "There was a problem when sending a message with CANWrite";
		m_signalErrorMessage( anaCallReturn, os.str().c_str() );
	}
	return ( true ); //sendAnErrorMessage(anaCallReturn);
}

void AnaCanScan2::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_baudRate);
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}


/**
 * the "ideal CAN bridge" logs. We get the n newest logs, n=0 for all
 * Ech log entry has a timestamp and a message, both as strings. The hw timestamp is in
 * fact an uint64 which we convert into a human readable string.
 */
std::vector<CanModule::PORT_LOG_ITEM_t> AnaCanScan2::getHwLogMessages ( unsigned int n ){
	std::vector<CanModule::PORT_LOG_ITEM_t> log;
	CanModule::PORT_LOG_ITEM_t item = Diag::createEmptyItem();
	AnaUInt32 nLogID = 0;
	AnaUInt32 pnCurrentID;
	AnaUInt32 pnLogCount;
	AnaInt64 pnLogDate;
	char pcBuffer[ 110 ];

	// first call to get the nb of logs
	AnaInt32 ret = CANGetLog( m_UcanHandle, nLogID, &pnCurrentID, &pnLogCount, &pnLogDate, pcBuffer );
	if ( ret != ANA_ERR_NONE ){
		MLOGANA2(ERR,this) << "There was a problem getting the HW logs " << ret << " . Abandoning HW log retrieval ";
		std::stringstream os;
		os << __FUNCTION__ 	<< "There was a problem getting the HW logs " << ret << " . Abandoning HW log retrieval ";
		m_signalErrorMessage( ret, os.str().c_str() );
		return log;
	}

	// how many can we return?
	unsigned int nLogs = 0;
	if ( pnLogCount >= n ){
		nLogs = n;
	} else {
		nLogs = pnLogCount;
	}
	if ( n == 0 ) {
		nLogs = pnLogCount;
	}
	nLogs--; // the pnLogCount counter points to the next log to write. FW error
	MLOGANA2(TRC,this) << "we can retrieve " << nLogs << " logs from the hw, with pnLogCount= " << pnLogCount;
	nLogID = pnCurrentID;
	for ( unsigned int i = 0; i < nLogs; i++ ){
		AnaInt32 ret0 = CANGetLog( m_UcanHandle, nLogID, &pnCurrentID, &pnLogCount, &pnLogDate, pcBuffer );
		if ( ret0 != ANA_ERR_NONE ){
			MLOGANA2(ERR,this) << "There was a problem getting HW log nLogID= " << nLogID << " ..skipping this entry" <<  ret ;
			std::stringstream os;
			os << __FUNCTION__ 	<< "There was a problem getting the HW logs " << ret;
			m_signalErrorMessage( ret, os.str().c_str() );
			nLogID++;
		} else {
			item.message = std::string ( pcBuffer );

			std::tm tm = *std::localtime( &pnLogDate );
			std::stringstream out;
			out << std::put_time( &tm, "%c %Z" );
			item.timestamp = out.str();

			MLOGANA2(TRC,this) << "found log item i= " << i << " nLogID= " << nLogID << " " << item.timestamp << " " << item.message << ret;
			log.push_back( item );
			nLogID++;
		}
	}
	return log;
}

/**
 * get diagnostic data and the client list plus connected ips.
 */
CanModule::HARDWARE_DIAG_t AnaCanScan2::getHwDiagnostics (){
	std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << std::endl;
	HARDWARE_DIAG_t d = CanModule::Diag::createEmptyDiag();
	const unsigned int sz = 6;
	unsigned int ips[ sz ];
	unsigned int ports[ sz ];
	AnaInt64 dates[ sz ];

	for ( unsigned int i = 0; i < sz; i++ ){
		ips[ i ] = 0;
		ports[ i ] = 0;
		dates[ i  ] = 0;
	}

	AnaInt32 p0, p1;
	AnaInt32 ret = CANGetDiagData( m_UcanHandle, &p0, &p1 );
	std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " ret= 0x" << std::hex << ret << std::dec << std::endl;
	d.temperature = (float) p0/10;
	d.uptime = p1;

	AnaInt32 ret1 = CANGetClientList( m_UcanHandle, &d.clientCount, ips, ports, dates );
	std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " ret1= 0x" << std::hex << ret1 << std::dec << std::endl;
	if ( ret != ANA_ERR_NONE || ret1 != ANA_ERR_NONE ){
		MLOGANA2(ERR,this) << "There was a problem getting the HW DiagData and/or client list " << ret << " . Abandoning HW DiagData and client list retrieval ";
		std::stringstream os;
		os << __FUNCTION__ 	<< "There was a problem getting the HW DiagData and/or client list" << ret << " . Abandoning HW DiagData and client list retrieval ";
		m_signalErrorMessage( ret, os.str().c_str() );
		return Diag::createEmptyDiag();
	}
	for ( unsigned int i = 0; i < sz; i++ ){
		d.clientIps.push_back( m_decodeNetworkByteOrder_ip_toString( ips[ i ] ) );
		d.clientPorts.push_back( ports[ i ] );

		std::tm tm = *std::localtime( &dates[ i ] );
		std::stringstream out;
		out << std::put_time( &tm, "%c %Z" );
		d.clientConnectionTimestamps.push_back( out.str() );
	}
	return d;
}


/**
 * decodes  client IP addresse in network byte order, e.g. 0x0201A8C0 ( 0x 02.01.A8.C0 ), into 192.168.1.2
 */
std::string AnaCanScan2::m_decodeNetworkByteOrder_ip_toString( unsigned int nip ){
	std::stringstream os;
	os << std::dec << (nip & 0xff) << "." << ((nip & 0xff00)>>8) << "." << ((nip & 0xff0000)>>16) << "." << ((nip & 0xff000000)>>24);
	std::string sip = os.str();
	// MLOGANA2(TRC,this) << __FUNCTION__ << " nip= " << nip << " sip= " << sip;
	return sip;
}

CanModule::PORT_COUNTERS_t AnaCanScan2::getHwCounters (){
	PORT_COUNTERS_t c;
	AnaInt32 ret = CANGetCounters( m_UcanHandle, &c.countTCPRx,
		&c.countTCPTx, &c.countCANRx, &c.countCANTx, &c.countCANRxErr, &c.countCANTxErr,
		&c.countCANRxDisc, &c.countCANTxDisc, &c.countCANTimeout);
	if ( ret != ANA_ERR_NONE ){
		MLOGANA2(ERR,this) << "There was a problem getting the HW counters " << ret << " . Abandoning HW counters retrieval ";
		std::stringstream os;
		os << __FUNCTION__ 	<< "There was a problem getting the HW counters " << ret << " . Abandoning HW counters retrieval ";
		m_signalErrorMessage( ret, os.str().c_str() );
		return Diag::createEmptyCounters();
	}
	return c;
}

/**
 * Reconnection thread managing the reconnection behavior, per port. The behavior settings can not change during runtime.
 * This thread is initialized after the main thread is up, and then listens on its cond.var as a trigger.
 * Triggers occur in two contexts: sending and receiving problems.
 * If there is a sending problem which lasts for a while (usually) the reconnection thread will be also triggered for each failed sending:
 * the thread will be "hammered" by triggers. ince the reconnection takes some time, many triggers will be lost. That is in fact a desired behavior.
 *
 * The parameters are all atomics for increased thread-safety, even though the documentation about the predicate is unclear on that point. Since
 * atomics just provide a "sequential memory layout" for the variables to prevent race conditions they are good to use for this but the code still has to be threadsafe
 * and reentrant... ;-) Doesn't eat anything anyway on that small scale with scalars only.
 *
 * https://en.cppreference.com/w/cpp/thread/condition_variable/wait
 */
void AnaCanScan2::m_CanReconnectionThread()
{
	std::string _tid;
	{
		std::stringstream ss;
		ss << std::this_thread::get_id();
		_tid = ss.str();
	}
	MLOGANA2(TRC, this ) << "created reconnection thread tid= " << _tid;

	// need some sync to the main thread to be sure it is up and the sock is created: wait first time for init
	waitForReconnectionThreadTrigger();

	/**
	 * lets check the timeoutOnReception reconnect condition. If it is true, all we can do is to
	 * close/open the socket again since the underlying hardware is hidden by socketcan abstraction.
	 * Like his we do not have to pollute the "sendMessage" like for anagate, and that is cleaner.
	 */
	MLOGANA2(TRC, this) << "initialized reconnection thread tid= " << _tid << ", entering loop";
	while ( true ) {

		// wait for sync: need a condition sync to step that thread once: a "trigger".
		MLOGANA2(TRC, this) << "waiting reconnection thread tid= " << _tid;
		waitForReconnectionThreadTrigger();
		MLOGANA2(TRC, this)
			<< " reconnection thread tid= " << _tid
			<< " condition "<< reconnectConditionString(getReconnectCondition() )
			<< " action " << reconnectActionString(getReconnectAction())
			<< " is checked, m_failedSendCountdown= "
			<< m_failedSendCountdown;


		/**
		 * just manage the conditions, and continue/skip if there is nothing to do
		 */
		switch ( getReconnectCondition() ){
		// the timeout is already checked before the send
		case CanModule::ReconnectAutoCondition::timeoutOnReception: {
			resetSendFailedCountdown(); // do the action
			break;
		}
		case CanModule::ReconnectAutoCondition::sendFail: {
			resetTimeoutOnReception();
			if (m_failedSendCountdown > 0) {
				continue;// do nothing
			} else {
				resetSendFailedCountdown(); // do the action
			}
			break;
		}
		// do nothing but keep counter and timeout resetted
		case CanModule::ReconnectAutoCondition::never:
		default:{
			resetSendFailedCountdown();
			resetTimeoutOnReception();
			continue;// do nothing
			break;
		}
		} // switch

		// single bus reset if (send) countdown or the (receive) timeout says so. reception timeout is already checked for anagate
		// to avoid lots of useless triggers at each send message.
		switch ( getReconnectAction() ){
		case CanModule::ReconnectAction::singleBus: {
			MLOGANA2(INF, this) << " reconnect condition " << reconnectConditionString(m_reconnectCondition)
										<< " triggered action " << reconnectActionString(m_reconnectAction);

			AnaInt32 ret = m_reconnectThisPort();
			MLOGANA2(INF, this) << "reconnected one CAN port ip= " << m_canIPAddress << " ret= " << ret;
			AnaCanScan2::st_showAnaCanScanObjectMap();
			break;
		}

		/**
		 * anagate (physical) modules can have more than one ethernet RJ45. we treat each ip address as "one bridge" in this context
		 *
		 * CanModule::ReconnectAction::allBusesOnBridge
		 */
		case CanModule::ReconnectAction::allBusesOnBridge: {
			std::string ip = m_ipAddress();
			MLOGANA2(INF, this) << " reconnect condition " << reconnectConditionString(m_reconnectCondition)
										<< " triggered action " << reconnectActionString(m_reconnectAction);
			AnaCanScan2::st_reconnectAllPorts( ip );
			MLOGANA2(TRC, this) << " reconnect all ports of ip=  " << ip << this->getBusName();
			break;
		}
		default: {
			// we have a runtime bug
			MLOGANA2(ERR, this) << "reconnection action "
					<< (int) m_reconnectAction << reconnectActionString( m_reconnectAction )
					<< " unknown. Check your config & see documentation. No action.";
			break;
		}
		} // switch
	} // while
}


