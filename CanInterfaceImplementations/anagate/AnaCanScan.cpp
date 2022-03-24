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

#include "AnaCanScan.h"

#include <thread>
#include <mutex>
#include <condition_variable>

#include <time.h>
#include <string.h>
#include <map>
#include <LogIt.h>
#include <sstream>
#include <iostream>

#include "CanModuleUtils.h"

#ifdef _WIN32

#define DLLEXPORTFLAG __declspec(dllexport)

#else

#include <sys/time.h>

#define DLLEXPORTFLAG  
#define WINAPI  

#endif


using namespace CanModule;
using namespace std;

//boost::mutex anagateReconnectMutex;
std::recursive_mutex anagateReconnectMutex;

/* static */ std::map<int, AnaCanScan::ANAGATE_PORTDEF_t> AnaCanScan::st_canHandleMap; // map handles to  {ports, ip}
/* static */ Log::LogComponentHandle AnaCanScan::st_logItHandleAnagate = 0;
/* static */ std::map<string,bool> AnaCanScan::m_reconnectInProgress_map;

/** global map of connection-objects: the map-key is the handle. Since handles are allocated by the OS
 * the keys are getting changed as well when we reconnect, so that we do not keep the stale keys(=handles) in
 * the map at all.
 * The map is used in the (static) InternalCallback code straightforward and speedy, because we can get to the object by just
 * looking up the map using the key(=the handle). The map manipulations are protected by serialization of reconnects
 * with the anagateReconnectMutex. At RO access in the callbacks the map is NOT protected because we do not want to
 * serialize the callbacks and because it is a RO map access, which is "like atomic". Well - it does work in multithreads.. ;-)
 */
std::map<AnaInt32, AnaCanScan*> g_AnaCanScanObjectMap; // map handles to objects

#define MLOGANA(LEVEL,THIS) LOG( Log::( LEVEL ), AnaCanScan::st_logItHandleAnagate)) << __FUNCTION__ << " " << " anagate bus= " << ( THIS )->getBusName() << " "

AnaCanScan::AnaCanScan():
	m_canPortNumber(0),
	m_canIPAddress( (char *) string("192.168.1.2").c_str()),
	m_baudRate(0),
	m_idCanScanThread(0),
	m_canCloseDevice(false),
	m_busName(""),
	m_busParameters(""),
	m_UcanHandle(0),
	m_timeout ( 6000 ),
	m_busStopped( false )
{
	m_statistics.setTimeSinceOpened();
	m_statistics.beginNewRun();
	m_failedSendCountdown = m_maxFailedSendCount;

	/**
	 * start a reconnection thread
	 */
	m_hCanReconnectionThread = new std::thread( &AnaCanScan::CanReconnectionThread, this);
}

/**
 * Shut down can scan thread
 */
AnaCanScan::~AnaCanScan()
{
	stopBus();
	MLOGANA(DBG,this) << __FUNCTION__ <<" closed successfully";
}

void AnaCanScan::stopBus ()
{
	if (!m_busStopped){
		MLOGANA(TRC,this) << __FUNCTION__ << " stopping anagate m_busName= " <<  m_busName << " m_canPortNumber= " << m_canPortNumber;
		CANSetCallback(m_UcanHandle, 0);
		CANCloseDevice(m_UcanHandle);
		deleteCanHandleOfPortIp( m_canPortNumber, m_canIPAddress );
		eraseReceptionHandlerFromMap( m_UcanHandle );
	}
	m_busStopped = true;
}


/* static */ void AnaCanScan::setIpReconnectInProgress( string ip, bool flag ){
	// only called inside the locked mutex
	std::map<string,bool>::iterator it = AnaCanScan::m_reconnectInProgress_map.find( ip );

	if ( flag ){
		// mark as in progress if not yet exists/marked
		if ( it == AnaCanScan::m_reconnectInProgress_map.end() ) {
			AnaCanScan::m_reconnectInProgress_map.insert ( std::pair<string,bool>( ip, true ) );
		}
	} else {
		// erase existing if still exists/marked
		if ( it != AnaCanScan::m_reconnectInProgress_map.end() ) {
			AnaCanScan::m_reconnectInProgress_map.erase( it );
		}
	}
}

/**
 * checks if a reconnection is already in progress for the bridge at that IP. Several ports can (and will) fail
 * at the same time, but the bridge should be reset only once, and not again for each port.
 */
/* static */ bool AnaCanScan::isIpReconnectInProgress( string ip ){
	// only called inside the locked mutex
	std::map<string,bool>::iterator it = AnaCanScan::m_reconnectInProgress_map.find( ip );
	if ( it == AnaCanScan::m_reconnectInProgress_map.end() )
		return( false );
	else
		return( true );
}


/**
 * creates and returns anagate can access object
 */
extern "C" DLLEXPORTFLAG CCanAccess *getCanBusAccess()
{
	CCanAccess *canAccess = new AnaCanScan;
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

	canMsgCopy.c_time = convertTimepointToTimeval(currentTimeTimeval());

	// more hardcode debugging, leave it in
	//cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
	//		<< " anagate message reception hHandle= " << hHandle
	//		<< " nIdentifier= " << nIdentifier
	//		<< endl;
	// MLOGANA(TRC, g_AnaCanScanPointerMap[hHandle] ) << "read(): " << canMessageToString(canMsgCopy);

	LOG( Log::TRC ) << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
			<< " CanModule anagate " << AnaCanScan::canMessageToString(canMsgCopy);

	g_AnaCanScanObjectMap[hHandle]->callbackOnRecieve(canMsgCopy);
	g_AnaCanScanObjectMap[hHandle]->statisticsOnRecieve( nBufferLen );
}

/**
 * return a human readabale CAN message
 */
/* static */ std::string AnaCanScan::canMessageToString(CanMessage &f)
{
	std::string result;
	result =  "[id=0x"+CanModuleUtils::toHexString(f.c_id, 3, '0')+" ";
	if (f.c_id & f.c_rtr)
		result += "RTR ";
	result+="dlc=" + CanModuleUtils::toString(int(f.c_dlc)) + " data=[";

	for (int i=0; i<f.c_dlc; i++)
		result+= CanModuleUtils::toHexString((unsigned int)f.c_data[i], 2, '0')+" ";
	result += "]]";
	return result;
}

/**
 *  update statistics API
 */
void AnaCanScan::statisticsOnRecieve(int bytes)
{
	m_statistics.setTimeSinceReceived();
	m_statistics.onReceive( bytes );
}

/**
 *  callback API: sends a boost signal with payload
 */
void AnaCanScan::callbackOnRecieve(CanMessage& msg)
{
	canMessageCame( msg );
	if ( m_reconnectCondition == ReconnectAutoCondition::timeoutOnReception ){
		resetTimeoutOnReception();
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
int AnaCanScan::createBus(const string name,const string parameters)
{	
	m_busName = name;
	m_busParameters = parameters;

	// calling base class to get the instance from there
	Log::LogComponentHandle myHandle;
	LogItInstance* logItInstance = CCanAccess::getLogItInstance(); // actually calling instance method, not class

	// register anagate component for logging
	if ( !LogItInstance::setInstance(logItInstance)) {
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
				<< " could not set LogIt instance" << std::endl;
	}
	if (!logItInstance->getComponentHandle(CanModule::LogItComponentName, myHandle)){
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
				<< " could not get LogIt component handle for " << LogItComponentName << std::endl;
	}

	AnaCanScan::st_logItHandleAnagate = myHandle;
	int returnCode = configureCanBoard(name, parameters);
	if ( returnCode < 0 ) {
		return -1;
	}
	MLOGANA(DBG,this) << " OK, Bus created with name= " << name << " parameters= " << parameters;
	m_busStopped = false;
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
int AnaCanScan::configureCanBoard(const string name,const string parameters)
{
	MLOGANA(DBG, this) << "(user supplied) name= " << name << " parameters= " << parameters;
	vector<string> stringVector;
	stringVector = parseNameAndParameters(name, parameters);

	// we should decode 3 elements from this:0="an" for anagate library, 1=can port, 2=ip number
	// like  "an:0:128.141.159.194"
	if ( stringVector.size() != 3 ){
		MLOGANA(ERR, this) << " need exactly 3 elements delimited by \":\" like \"an:0:128.141.159.194\", got "
				<< stringVector.size();
		for ( unsigned i = 0; i < stringVector.size(); i++ ){
			MLOGANA(ERR, this) << " stringVector[" << i << "]= " << stringVector[ i ];
		}
		return(-1);
	} else {
		for ( unsigned i = 0; i < stringVector.size(); i++ ){
			MLOGANA(TRC, this) << "(cleaned up) stringVector[" << i << "]= " << stringVector[ i ];
		}
		m_canPortNumber = atoi(stringVector[1].c_str());
		m_canIPAddress = (char *) stringVector[2].c_str();
	}
	MLOGANA(TRC, this) << "(cleaned up) canPortNumber= " << m_canPortNumber << " ip= " << m_canIPAddress;

	// handle up to 7 parameter, assume defaults if needed
	m_baudRate = 125000L;

	if (strcmp(parameters.c_str(), "Unspecified") != 0) {
		MLOGANA(TRC, this) << "m_CanParameters.m_iNumberOfDetectedParameters " << m_CanParameters.m_iNumberOfDetectedParameters;
		if ( m_CanParameters.m_iNumberOfDetectedParameters >= 1 )	{
			m_baudRate = m_CanParameters.m_lBaudRate; // just for the statistics

			if (( m_CanParameters.m_lBaudRate > 125000L ) && ( ! m_CanParameters.m_iHighSpeed )){
				MLOGANA(WRN, this) << "baud rate is above 125000bits/s, and the high speed flag is not set, but needed. Setting high speed flag for you. To avoid this warning set the high speed flag properly in your configuration";
				m_CanParameters.m_iHighSpeed = 1;
			}

			if ( m_CanParameters.m_iNumberOfDetectedParameters >= 7 )	{
				m_timeout = m_CanParameters.m_iTimeout; // here we set it: CANT-44
				MLOGANA(TRC, this) << "anagate: picked up from 7th parameter: m_timeout= " << m_timeout;
			}

			// any other parameters are already set, either to 0 by init,
			// or by decoding. They are always used.
		} else {
			MLOGANA(ERR, this) << "Error while parsing parameters: this syntax is incorrect: [" << parameters << "]";
			MLOGANA(ERR, this) << "you need up to 7 numbers separated by whitespaces, i.e. \"125000 0 1 0 0 0 6000\" \"p0 p1 p2 p3 p4 p5 p6\"";
			MLOGANA(ERR, this) << "  p0 = baud rate, 125000 or whatever the module supports";
			MLOGANA(ERR, this) << "  p1 = operation mode";
			MLOGANA(ERR, this) << "  p2 = termination";
			MLOGANA(ERR, this) << "  p3 = high speed";
			MLOGANA(ERR, this) << "  p4 = time stamp";
			MLOGANA(ERR, this) << "  p5 = sync mode";
			MLOGANA(ERR, this) << "  p6 = timeout/ms";
			return -1;
		}
	} else	{
		// Unspecified: we use defaults "125000 0 1 0 0 0 6000"
		MLOGANA(INF, this) << "Unspecified parameters, default values (termination=1) will be used.";
		m_CanParameters.m_lBaudRate = 125000;
		m_CanParameters.m_iOperationMode = 0;
		m_CanParameters.m_iTermination = 1 ;// ENS-26903: default is terminated
		m_CanParameters.m_iHighSpeed = 0;
		m_CanParameters.m_iTimeStamp = 0;
		m_CanParameters.m_iSyncMode = 0;
		m_CanParameters.m_iTimeout = 6000; // CANT-44: can be set
	}
	MLOGANA(TRC, this) << __FUNCTION__ << " m_lBaudRate= " << m_CanParameters.m_lBaudRate;
	MLOGANA(TRC, this) << __FUNCTION__ << " m_iOperationMode= " << m_CanParameters.m_iOperationMode;
	MLOGANA(TRC, this) << __FUNCTION__ << " m_iTermination= " << m_CanParameters.m_iTermination;
	MLOGANA(TRC, this) << __FUNCTION__ << " m_iHighSpeed= " << m_CanParameters.m_iHighSpeed;
	MLOGANA(TRC, this) << __FUNCTION__ << " m_iTimeStamp= " << m_CanParameters.m_iTimeStamp;
	MLOGANA(TRC, this) << __FUNCTION__ << " m_iSyncMode= " << m_CanParameters.m_iSyncMode;
	MLOGANA(TRC, this) << __FUNCTION__ << " m_iTimeout= " << m_CanParameters.m_iTimeout;
	return openCanPort();
}

/**
 * add a can handle into the map if it does not yet exist
 */
/* static */ void AnaCanScan::addCanHandleOfPortIp(AnaInt32 handle, int port, std::string ip) {
	std::map<AnaInt32, ANAGATE_PORTDEF_t>::iterator it = st_canHandleMap.find( handle );
	if ( it != st_canHandleMap.end() ){
		LOG(Log::WRN, AnaCanScan::st_logItHandleAnagate) << __FUNCTION__
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
/* static */ void AnaCanScan::deleteCanHandleOfPortIp(int port, std::string ip){
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
		LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate) << __FUNCTION__
				<< " could not erase handle= " << handle
				<< " for port= " << port << " ip= " << ip;
	}
}

/**
 * find the handle from port, ip
 * we have to search through the whole map to know
 */
/* static */ AnaInt32 AnaCanScan::getCanHandleOfPortIp(int port, std::string ip) {

	LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate) << __FUNCTION__
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
int AnaCanScan::openCanPort()
{
	AnaInt32 anaCallReturn;
	AnaInt32 canModuleHandle;

	/**
	 * check if that anagate bridge (ip) is already initialized, if not we create it.
	 */
	canModuleHandle = getCanHandleOfPortIp(m_canPortNumber, m_canIPAddress );
	if ( canModuleHandle < 0 ){
		MLOGANA(DBG, this) << "calling CANOpenDevice with port= " << m_canPortNumber << " ip= " << m_canIPAddress
				<< " canModuleHandle= " << canModuleHandle
				<< " m_timeout= " << m_timeout;
		anaCallReturn = CANOpenDevice(&canModuleHandle, FALSE, TRUE, m_canPortNumber, m_canIPAddress.c_str(), m_timeout);
		if (anaCallReturn != 0) {
			// fill out initialisation struct
			MLOGANA(ERR,this) << "Error in CANOpenDevice, return code = [0x" << hex << anaCallReturn << dec << "]";
			return -1;
		}
	} else {
		MLOGANA(DBG, this) << "port= " << m_canPortNumber << " ip= " << m_canIPAddress
				<< " is open already, got handle= "
				<< canModuleHandle;
	}

	// initialize CAN interface
	MLOGANA(TRC,this) << "calling CANSetGlobals with m_lBaudRate= "
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
		MLOGANA(ERR,this) << "Connection to TCP/IP partner can't be established or is disconnected. Lost TCP/IP: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	case 0x40000: {
		MLOGANA(ERR,this) << "No  answer  was  received  from	TCP/IP partner within the defined timeout. Lost TCP/IP: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	case 0x900000: {
		MLOGANA(ERR,this) << "Invalid device handle. Lost TCP/IP: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	default : {
		MLOGANA(ERR,this) << "Other Error in CANSetGlobals: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	}
	// set handler for managing incoming messages
	MLOGANA(TRC,this) << "calling CANSetCallbackEx handle= " << canModuleHandle;
	anaCallReturn = CANSetCallbackEx(canModuleHandle, InternalCallback);
	if (anaCallReturn != 0) {
		MLOGANA(ERR,this) << "Error in CANSetCallbackEx, return code = [" << anaCallReturn << "]";
		return -1;
	} else {
		MLOGANA(TRC,this) << "OK CANSetCallbackEx, return code = [" << anaCallReturn << "]";
	}

	// bookkeeping in maps for {port, ip} and object pointers
	addCanHandleOfPortIp( canModuleHandle, m_canPortNumber, m_canIPAddress );
	g_AnaCanScanObjectMap[canModuleHandle] = this; // gets added
	m_UcanHandle = canModuleHandle;

	// keep some hardcore debugging
	// AnaCanScan::showAnaCanScanObjectMap();
	return 0;
}

/**
 * if sending had a problem invoke the error handler with a message.
 * Ultimately, this sends a boost::signal to a connected boost::slot in the client's code.
 */
bool AnaCanScan::sendAnErrorMessage(AnaInt32 status)
{
	char errorMessage[120];
	timeval ftTimeStamp; 
	if (status != 0) {
		auto now = std::chrono::system_clock::now();
		auto nMicrosecs =
				duration_cast<std::chrono::microseconds>(
						now.time_since_epoch()
				);
		ftTimeStamp.tv_sec = nMicrosecs.count() / 1000000L;
		ftTimeStamp.tv_usec = (nMicrosecs.count() % 1000000L) ;

		if (!errorCodeToString( (long int) status, errorMessage))
			canMessageError(status, errorMessage, ftTimeStamp);
	}
	return true;
}

/* static */ std::string AnaCanScan::canMessage2ToString(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	std::string result = "";
	result =  "[id=0x"+CanModuleUtils::toHexString(cobID, 3, '0')+" ";
	if ( rtr )
		result += "RTR ";
	result+="dlc= " + CanModuleUtils::toHexString( len ) + " (hex) data= ";

	for (int i=0; i< len; i++)
		result+= CanModuleUtils::toHexString((unsigned int) message[i], 2, '0')+" ";
	result += "]";
	return result;
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
bool AnaCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{

	if ( m_canCloseDevice || m_busStopped ){
		MLOGANA(WRN,this) << __FUNCTION__ << " bus is closed, skipping [ closed= " << m_canCloseDevice << " stopped= " << m_busStopped << "]";
		return( false );
	}

	// /* static */ std::string AnaCanScan::canMessageToString(CanMessage &f)
	// MLOGANA(DBG,this) << "Sending message: [" << ( message == 0  ? "" : (const char *) message) << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";

	MLOGANA(DBG,this) << "Sending message: [" << AnaCanScan::canMessage2ToString(cobID, len, message, rtr) << "]";
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
		MLOGANA(DBG, this) << "The length is more than 8 bytes, ignoring overhead " << len;
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
		MLOGANA(WRN, this) << " m_canPortNumber= " << m_canPortNumber << " skipping send, detected "
				<< reconnectConditionString(m_reconnectCondition);

		//send a reconnection thread trigger
		MLOGANA(DBG, this) << "trigger reconnection thread to check reception timeout " << getBusName();
		triggerReconnectionThread();

	} else {

		/**
		 *  we can send now
		 */
		memcpy(messageToBeSent, message, messageLengthToBeProcessed);
		MLOGANA(TRC, this) << " m_canPortNumber= " << m_canPortNumber
				<< " cobID= " << cobID
				<< " length = " << messageLengthToBeProcessed
				<< " flags= " << flags << " m_UcanHandle= " << m_UcanHandle;
		anaCallReturn = CANWrite(m_UcanHandle, cobID, reinterpret_cast<char*>(messageToBeSent), messageLengthToBeProcessed, flags);
	}

	if (anaCallReturn != 0) {
		MLOGANA(ERR, this) << "There was a problem when sending/receiving timeout with CANWrite or a reconnect condition: 0x"
				<< hex << anaCallReturn << dec
				<< " ip= " << m_canIPAddress;
		m_canCloseDevice = false;
		decreaseSendFailedCountdown();

		//send a reconnection thread trigger
		MLOGANA(DBG, this) << "trigger reconnection thread since a send failed " << getBusName();
		triggerReconnectionThread();
	} else {
		m_statistics.onTransmit(messageLengthToBeProcessed);
		m_statistics.setTimeSinceTransmitted();
	}
	return sendAnErrorMessage(anaCallReturn);
}

/**
 * display the handle->object map, and using the object, show more details
 */
/* static */ void AnaCanScan::showAnaCanScanObjectMap(){
	uint32_t size = g_AnaCanScanObjectMap.size();
	LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate  ) << __FUNCTION__ << " RECEIVE obj. map size= " << size << " : ";
	for (std::map<AnaInt32, AnaCanScan*>::iterator it=g_AnaCanScanObjectMap.begin(); it!=g_AnaCanScanObjectMap.end(); it++){
		LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate ) << __FUNCTION__ << " obj. map "
				<< " key= " << it->first
				<< " ip= " << it->second->ipAdress()
				<< " CAN port= " << it->second->canPortNumber()
				<< " handle= " << it->second->handle()
				<< " ptr= 0x" << hex << (unsigned long) (it->second) << dec; // naja, just to see it tis != NULL

	}
}


/**
 * only reconnect this object's port and make dure the callback map entry is updated as well
 */
AnaInt32 AnaCanScan::reconnectThisPort(){
	// a sending on one port failed miserably, and we reset just that port.
	AnaInt32 anaCallReturn0 = reconnect();
	while ( anaCallReturn0 < 0 ){
		MLOGANA(WRN, this) << "failed to reconnect CAN port " << m_canPortNumber
				<< " ip= " << m_canIPAddress << ",  try again";
		CanModule::ms_sleep( 5000 );
		anaCallReturn0 = reconnect();
	}
	showAnaCanScanObjectMap();
	anaCallReturn0 = connectReceptionHandler();
	LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate ) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
			<< " reconnect reception handler for port= " << m_canPortNumber
			<< " ip= " << m_canIPAddress
			<< " looking good= " << anaCallReturn0;

	// erase stale map entry for the old reception handler
	std::map<AnaInt32, AnaCanScan*> lmap = g_AnaCanScanObjectMap;
	for (std::map<AnaInt32, AnaCanScan*>::iterator it=lmap.begin(); it!=lmap.end(); it++){
		/**
		 * delete the map elements where the key is different from the handle, since we reassigned handles
		 * in the living objects like:
		 * 	g_AnaCanScanPointerMap[ m_UcanHandle ] = this;
		 */
		if ( it->first != it->second->handle()){
			LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
					<< " erasing stale handler " << it->first
					<< " for object handle= " << it->second->handle() << " from obj. map";
			g_AnaCanScanObjectMap.erase( it->first );
		}
	}
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
/* static */ AnaInt32 AnaCanScan::reconnectAllPorts( string ip ){

	// protect against several calls on the same ip, recursive_mutex needed
	{
		anagateReconnectMutex.lock();
		if ( AnaCanScan::isIpReconnectInProgress( ip ) ) {
			LOG(Log::WRN, AnaCanScan::st_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
					<< " is already in progress, skipping.";
			CanModule::ms_sleep( 10000 );
			anagateReconnectMutex.unlock();
			return(1);
		}
		AnaCanScan::setIpReconnectInProgress( ip, true );
		LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
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
	for (std::map<AnaInt32, AnaCanScan*>::iterator it=g_AnaCanScanObjectMap.begin(); it!=g_AnaCanScanObjectMap.end(); it++){
		if ( ip == it->second->ipAdress() ){
			LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate ) << __FUNCTION__
					<< " key= " << it->first
					<< " found ip= " << ip
					<< " for CAN port= " << it->second->canPortNumber()
					<< " reconnecting...";

			ret = it->second->reconnect();
			if ( ret ){
				LOG(Log::WRN, AnaCanScan::st_logItHandleAnagate) << __FUNCTION__
						<< " key= " << it->first
						<< " found ip= " << ip
						<< " for CAN port= " << it->second->canPortNumber()
						<< " failed to reconnect";
				reconnectFailed = true;
			}
			nbCANportsOnModule++;
		}
	}
	LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate) << " CAN bridge at ip= " << ip << " uses  nbCANportsOnModule= " << nbCANportsOnModule;

	if ( reconnectFailed ) {
		LOG(Log::WRN, AnaCanScan::st_logItHandleAnagate ) << " Problem reconnecting CAN ports for ip= " << ip
				<< " last ret= " << ret << ". Just abandoning and trying again in 10 secs, module might not be ready yet.";

		CanModule::ms_sleep( 10000 );

		AnaCanScan::setIpReconnectInProgress( ip, false );
		LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
				<< " cancel.";
		return(-1);
	}

	// register the new handler with the obj. map and connect the reception handler. Cleanup the stale handlers
	// use a local copy of the map, in order not to change the map we are iterating on
	std::map<AnaInt32, AnaCanScan*> lmap = g_AnaCanScanObjectMap;
	LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
			<< " receive handler map map before reconnect for ip= " << ip;
	AnaCanScan::showAnaCanScanObjectMap();

	for (std::map<AnaInt32, AnaCanScan*>::iterator it=lmap.begin(); it!=lmap.end(); it++){
		if ( ip == it->second->ipAdress() ){
			anaRet = it->second->connectReceptionHandler();
			if ( anaRet != 0 ){
				LOG(Log::ERR, AnaCanScan::st_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
						<< " failed to reconnect reception handler for ip= " << ip
						<< " handle= " << it->second->handle()
						<< " port= " << it->second->canPortNumber()
						<< " anaRet= " << anaRet;
			} else {
				LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate ) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
						<< " reconnect reception handler for ip= " << ip
						<< " handle= " << it->second->handle()
						<< " looking good= " << anaRet;
			}

			/**
			 * delete the map elements where the key is different from the handle, since we reassigned handles
			 * in the living objects like:
			 * 	g_AnaCanScanPointerMap[ m_UcanHandle ] = this;
			 */
			if ( it->first != it->second->handle()){
				LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
						<< " erasing stale handler " << it->first
						<< " for object handle= " << it->second->handle() << " from obj. map";
				g_AnaCanScanObjectMap.erase( it->first );
			}
			AnaCanScan::setIpReconnectInProgress( ip, false ); // all done, may fail another time
			LOG(Log::INF, AnaCanScan::st_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
					<< " is done and OK.";
		}
	}
	//LOG(Log::TRC, AnaCanScan::st_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
	//		<< " receive handler map after reconnect for ip= " << ip;
	AnaCanScan::showAnaCanScanObjectMap();

	// be easy on the switch
	CanModule::ms_sleep( 100 );
	return( anaRet );
}

/**
 * connect the reception handler and only AFTER that register the handle
 * in the global obj. map, since reception is asynchron.
 * Finally, in the caller, the stale handle is deleted from the map.
 */
AnaInt32 AnaCanScan::connectReceptionHandler(){
	AnaInt32 anaCallReturn;

	// this is needed otherwise the bridge hangs in a bad state
	anaCallReturn = CANSetGlobals(m_UcanHandle, m_CanParameters.m_lBaudRate, m_CanParameters.m_iOperationMode,
			m_CanParameters.m_iTermination, m_CanParameters.m_iHighSpeed, m_CanParameters.m_iTimeStamp);
	switch ( anaCallReturn ){
	case 0:{ break; }
	case 0x30000: {
		MLOGANA(ERR,this) << "Connection to TCP/IP partner can't be established or is disconnected. Lost TCP/IP: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	case 0x40000: {
		MLOGANA(ERR,this) << "No  answer  was  received  from	TCP/IP partner within the defined timeout. Lost TCP/IP: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	case 0x900000: {
		MLOGANA(ERR,this) << "Invalid device handle. Lost TCP/IP: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	default : {
		MLOGANA(ERR,this) << "Other Error in CANSetGlobals: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	}

	MLOGANA(WRN,this) << "connecting RECEIVE canModuleHandle= " << m_UcanHandle
			<< " ip= " << m_canIPAddress;
	anaCallReturn = CANSetCallbackEx(m_UcanHandle, InternalCallback);
	if (anaCallReturn != 0) {
		MLOGANA(ERR,this) << "failed CANSetCallbackEx, return code = [" << anaCallReturn << "]"
				<< " canModuleHandle= " << m_UcanHandle
				<< " in a reconnect! Can't fix this, maybe hardware/firmware problem.";
		// that is very schlecht, need a good idea (~check keepalive and sending fake messages)
		// to detect and possibly recuperate from that. Does this case happen actually?
	}
	MLOGANA(DBG,this) << "adding RECEIVE handler, handle= " << m_UcanHandle
			<< " CAN port= " << m_canPortNumber
			<< " ip= " << m_canIPAddress << " to obj.map.";
	g_AnaCanScanObjectMap[ m_UcanHandle ] = this;
	// setCanHandleInUse( m_UcanHandle, true );

	MLOGANA(DBG,this) << "RECEIVE handler looks good, handle= " << m_UcanHandle
			<< " CAN port= " << m_canPortNumber
			<< " ip= " << m_canIPAddress << " Congratulations.";
	return( anaCallReturn );
}

/**
 * todo: protect with mutex, work on copy
 */
void AnaCanScan::eraseReceptionHandlerFromMap( AnaInt32 h ){
	std::map<AnaInt32, AnaCanScan *>::iterator it = g_AnaCanScanObjectMap.find( h );
	if (it != g_AnaCanScanObjectMap.end()) {
		g_AnaCanScanObjectMap.erase ( it );
		m_busName = "nobus";
	} else {
		MLOGANA(TRC,this) << " handler " << h << " not found in map, not erased";
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
int AnaCanScan::reconnect(){
	AnaInt32 anaCallReturn;
	AnaInt32 canModuleHandle;

	// we are not too fast. sleep to wait for network and don't hammer the switch
	CanModule::ms_sleep( 100 );

	MLOGANA(WRN, this) << "try to reconnect ip= " << m_canIPAddress
			<< " m_canPortNumber= " << m_canPortNumber
			<< " m_UcanHandle= " << m_UcanHandle;

	int state = 5; // re-open completely unless otherwise
	if ( m_UcanHandle > 0 ){
		m_canCloseDevice = false;
	} else {
		// todo: we should probably drop this, if the handle isn't valid anyway
		state = CANDeviceConnectState( m_UcanHandle );
	}
	MLOGANA(TRC, this) << "CANDeviceConnectState: device connect state= 0x" << hex << state << dec;
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
		MLOGANA(INF,this) << "device is in state CONNECTING, don't try to reconnect for now, skip.";
		break;
	}

	case 3: {
		MLOGANA(INF,this) << "device is in state CONNECTED, don't try to reconnect, just skip.";
		break;
	}
	default:
	case 1:
	case 4:
	case 5:{
		if ( !m_canCloseDevice ) {
			anaCallReturn = CANCloseDevice( m_UcanHandle );
			MLOGANA(WRN, this) << "closed device m_UcanHandle= " << m_UcanHandle
					<< " anaCallReturn= 0x" << hex << anaCallReturn << dec;
			if ( anaCallReturn != 0 ) {
				MLOGANA(WRN, this) << "could not close device m_UcanHandle= " << m_UcanHandle
						<< " anaCallReturn= 0x" << hex << anaCallReturn << dec;
				// return(-3);
			}
			m_canCloseDevice = true;
			MLOGANA(TRC, this) << "device is closed. stale handle= " << m_UcanHandle;
		} else {
			MLOGANA(WRN, this) << "device is already closed, try reopen. stale handle= " << m_UcanHandle;
		}
		//setCanHandleInUse( m_UcanHandle, false );
		deleteCanHandleOfPortIp( m_canPortNumber, m_canIPAddress );

		MLOGANA(TRC,this) << "try CANOpenDevice m_canPortNumber= " << m_canPortNumber
				<< " m_UcanHandle= " << m_UcanHandle
				<< " ip= " << m_canIPAddress << " timeout= " << m_timeout;
		anaCallReturn = CANOpenDevice(&canModuleHandle, FALSE, TRUE, m_canPortNumber, m_canIPAddress.c_str(), m_timeout);
		if (anaCallReturn != 0) {
			MLOGANA(ERR,this) << "CANOpenDevice failed: 0x" << hex << anaCallReturn << dec;
			m_canCloseDevice = false;
			return(-1);
		}
		MLOGANA(DBG,this) << "CANOpenDevice m_canPortNumber= " << m_canPortNumber
				<< " canModuleHandle= " << canModuleHandle
				<< " ip= " << m_canIPAddress << " timeout= " << m_timeout << " reconnect for SEND looks good";

		addCanHandleOfPortIp( canModuleHandle, m_canPortNumber, m_canIPAddress );
		m_UcanHandle = canModuleHandle;
		m_canCloseDevice = false;
		// object in object map stays the same
		break;
	}
	}
	CanModule::ms_sleep( 100 );
	return( 0 ); // OK
}

#if 0
/**
 *
 * testing/debugging stuff. keep it in for the moment
 *
 * set a connection specific timeout value.
 * Global timeout is 6000, set by CanOpenDevice
 * In order to make this specific timeout apply we have to reconnect
 */
void AnaCanScan::setTimeoutAndReconnect( int timeout_ms ){
	m_timeout = timeout_ms;
	MLOGANA(WRN,this) << "reconnect CANOpenDevice m_UcanHandle= " << m_UcanHandle << " to apply new timeout= " << m_timeout;
	reconnect();
}
#endif

bool AnaCanScan::sendMessage(CanMessage *canm)
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
bool AnaCanScan::sendRemoteRequest(short cobID)
{
	AnaInt32 anaCallReturn;
	AnaInt32 flags = 2;// Bit 1: If set, the telegram is marked as remote frame.

	anaCallReturn = CANWrite(m_UcanHandle,cobID,NULL, 0, flags);
	if (anaCallReturn != 0) {
		MLOGANA(ERR,this) << "There was a problem when sending a message with CANWrite";
	}
	return sendAnErrorMessage(anaCallReturn);
}

/**
 * give back the error message fro the code, from appendixA
 */
std::string AnaCanScan::ana_canGetErrorText( long errorCode ){
	switch( errorCode ){
	case ANA_ERR_NONE: return("No errors");
	case ANA_ERR_OPEN_MAX_CONN: return("Open failed, maximal count of connections reached.");
	case ANA_ERR_OP_CMD_FAILED: return("Command failed with unknown failure");
	case ANA_ERR_TCPIP_SOCKET: return("Socket error in TCP/IP layer occured.");
	case ANA_ERR_TCPIP_NOTCONNECTED: return("Connection to TCP/IP partner can't\
			established or is disconnected.");
	case ANA_ERR_TCPIP_TIMEOUT: return("No answer received from TCP/IP\
            partner within the defined timeout");
	case ANA_ERR_TCPIP_CALLNOTALLOWED: return("Command is not allowed at this time.");
	case ANA_ERR_TCPIP_NOT_INITIALIZED: return("TCP/IP-Stack can't be initialized.");
	case ANA_ERR_INVALID_CRC: return("AnaGate TCP/IP telegram has incorrect checksum (CRC).");
	case ANA_ERR_INVALID_CONF: return("AnaGate TCP/IP telegram wasn't\
            receipted from partner.");
	case ANA_ERR_INVALID_CONF_DATA: return("AnaGate TCP/IP telegram wasn't\
            receipted correct from partner.");
	case ANA_ERR_INVALID_DEVICE_HANDLE: return("Invalid device handle");
	case ANA_ERR_INVALID_DEVICE_TYPE: return("Function can't be executed on this\
			device handle, as she is assigned\
			to another device type of AnaGate series.");
	}
	return("unknown error code");
}

/**
 * Provides textual representation of an error code.
 * error return from module
 */
bool AnaCanScan::errorCodeToString(long error, char message[])
{
	std::string ss = ana_canGetErrorText( error );
	message = new char[512];
	strcpy(message, ss.c_str());
	return true;
}

void AnaCanScan::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_baudRate);
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
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
void AnaCanScan::CanReconnectionThread()
{
	std::string _tid;
	{
		std::stringstream ss;
		ss << this_thread::get_id();
		_tid = ss.str();
	}
	MLOGANA(TRC, this ) << "created reconnection thread tid= " << _tid;

	// need some sync to the main thread to be sure it is up and the sock is created: wait first time for init
	waitForReconnectionThreadTrigger();

	/**
	 * lets check the timeoutOnReception reconnect condition. If it is true, all we can do is to
	 * close/open the socket again since the underlying hardware is hidden by socketcan abstraction.
	 * Like his we do not have to pollute the "sendMessage" like for anagate, and that is cleaner.
	 */
	MLOGANA(TRC, this) << "initialized reconnection thread tid= " << _tid << ", entering loop";
	while ( true ) {

		// wait for sync: need a condition sync to step that thread once: a "trigger".
		MLOGANA(TRC, this) << "waiting reconnection thread tid= " << _tid;
		waitForReconnectionThreadTrigger();
		MLOGANA(TRC, this)
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
			MLOGANA(INF, this) << " reconnect condition " << reconnectConditionString(m_reconnectCondition)
										<< " triggered action " << reconnectActionString(m_reconnectAction);

			AnaInt32 ret = reconnectThisPort();
			MLOGANA(INF, this) << "reconnected one CAN port ip= " << m_canIPAddress << " ret= " << ret;
			showAnaCanScanObjectMap();
			break;
		}

		/**
		 * anagate (physical) modules can have more than one ethernet RJ45. we treat each ip address as "one bridge" in this context
		 *
		 * CanModule::ReconnectAction::allBusesOnBridge
		 */
		case CanModule::ReconnectAction::allBusesOnBridge: {
			std::string ip = ipAdress();
			MLOGANA(INF, this) << " reconnect condition " << reconnectConditionString(m_reconnectCondition)
										<< " triggered action " << reconnectActionString(m_reconnectAction);
			AnaCanScan::reconnectAllPorts( ip );
			MLOGANA(TRC, this) << " reconnect all ports of ip=  " << ip << this->getBusName();
			break;
		}
		default: {
			// we have a runtime bug
			MLOGANA(ERR, this) << "reconnection action "
					<< (int) m_reconnectAction << reconnectActionString( m_reconnectAction )
					<< " unknown. Check your config & see documentation. No action.";
			break;
		}
		} // switch
	} // while
}


