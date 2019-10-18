/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * AnaCanScan.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: pnikiel, vfilimon, quasar team
 *      mludwig at cern dot ch
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

boost::mutex anagateReconnectMutex;

/* static */ bool AnaCanScan::s_isCanHandleInUseArray[256];
/* static */ AnaInt32 AnaCanScan::s_canHandleArray[256];
/* static */ bool AnaCanScan::s_logItRegisteredAnagate = false;
/* static */ Log::LogComponentHandle AnaCanScan::s_logItHandleAnagate = 0;
/* static */ std::map<string,bool> AnaCanScan::reconnectInProgress_map;

#define MLOGANA(LEVEL,THIS) LOG(Log::LEVEL, AnaCanScan::s_logItHandleAnagate) << __FUNCTION__ << " " << " anagate bus= " << THIS->getBusName() << " "

/** global map of connection-object-pointers: the map-key is the handle. Since handles are allocated by the OS
 * the keys are getting changed as well when we reconnect, so that we do not keep the stale keys(=handles) in
 * the map at all.
 * The map the InternalCallback code straightforward and speedy, because we can get to the object by just
 * looking up the map using the key(=the handle).
 */
std::map<AnaInt32, AnaCanScan*> g_AnaCanScanPointerMap;

AnaCanScan::AnaCanScan():
				m_canPortNumber(0),
				m_canIPAddress( (char *) string("192.168.1.2").c_str()),
				m_baudRate(0),
				m_idCanScanThread(0),
				m_canCloseDevice(false),
				m_busName(""),
				m_busParameters(""),
				m_UcanHandle(0),
				m_timeout ( 6000 )
{
	m_statistics.beginNewRun();
}

/**
 * Shut down can scan thread
 */
AnaCanScan::~AnaCanScan()
{
	stopBus();
}

void AnaCanScan::stopBus ()
{
	MLOGANA(TRC,this) << __FUNCTION__ << " m_busName= " <<  m_busName << " m_canPortNumber= " << m_canPortNumber;
	CANSetCallback(m_UcanHandle, 0);
	CANCloseDevice(m_UcanHandle);

	setCanHandleInUse(m_canPortNumber,false);


#if 0
	// notify the thread that it should finish.
	m_CanScanThreadRunEnableFlag = false;
	MLOGANA(DBG,this) << " try joining threads...";

	std::map<string, string>::iterator it = CSockCanScan::m_busMap.find( m_busName );
	if (it != CSockCanScan::m_busMap.end()) {
		pthread_join( m_hCanScanThread, 0 );
		m_idCanScanThread = 0;
		CSockCanScan::m_busMap.erase ( it );
		m_busName = "nobus";
	} else {
		MLOGANA(DBG,this) << " not joining threads... bus does not exist";
	}
#endif
	MLOGANA(TRC,this) << __FUNCTION__ << " finished";
}




/* static */ void AnaCanScan::setIpReconnectInProgress( string ip, bool flag ){
	std::map<string,bool>::iterator it = AnaCanScan::reconnectInProgress_map.find( ip );

	if ( flag ){
		// mark as in progress if not yet exists/marked
		if ( it == AnaCanScan::reconnectInProgress_map.end() ) {
			AnaCanScan::reconnectInProgress_map.insert ( std::pair<string,bool>( ip, true ) );
		}
	} else {
		// erase existing if still exists/marked
		if ( it != AnaCanScan::reconnectInProgress_map.end() ) {
			AnaCanScan::reconnectInProgress_map.erase( it );
		}
	}
}

/* static */ bool AnaCanScan::isIpReconnectInProgress( string ip ){
	std::map<string,bool>::iterator it = AnaCanScan::reconnectInProgress_map.find( ip );
	if ( it == AnaCanScan::reconnectInProgress_map.end() )
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

	//cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
	//		<< " anagate message reception hHandle= " << hHandle
	//		<< " nIdentifier= " << nIdentifier
	//		<< endl;

	g_AnaCanScanPointerMap[hHandle]->callbackOnRecieve(canMsgCopy);
	g_AnaCanScanPointerMap[hHandle]->statisticsOnRecieve( nBufferLen );
}


/**
 *  callback API
 */
void AnaCanScan::statisticsOnRecieve(int bytes)
{
	m_statistics.onReceive( bytes );
}

/**
 *  callback API
 */
void AnaCanScan::callbackOnRecieve(CanMessage& msg)
{
	canMessageCame( msg );
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
 * 				* p4: TimeStamp: 0=deactivated (default), 1=activated. If activated, a timestamp is added to the CAN frame. Not all modules support this.
 *				  i.e. "250000 0 1 0 0"
 * 				(see anagate manual for more details)
 *
 * @return was the initialisation process successful?
 */
bool AnaCanScan::createBus(const string name,const string parameters)
{	
	m_busName = name;
	m_busParameters = parameters;

	// calling base class to get the instance from there
	Log::LogComponentHandle myHandle;
	LogItInstance* logItInstance = CCanAccess::getLogItInstance(); // actually calling instance method, not class
	// std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " ptr= 0x" << logItInstance << std::endl;

	// register anagate component for logging
	if ( !LogItInstance::setInstance(logItInstance))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not set LogIt instance" << std::endl;


	if (!logItInstance->getComponentHandle(CanModule::LogItComponentName, myHandle))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not get LogIt component handle for " << LogItComponentName << std::endl;

	//LOG(Log::TRC, myHandle) << __FUNCTION__ << " " __FILE__ << " " << __LINE__;
	AnaCanScan::s_logItHandleAnagate = myHandle;

	m_sBusName = name;
	MLOGANA(DBG, this) << " parameters= " << parameters;
	int returnCode = configureCanBoard(name, parameters);
	if ( returnCode < 0 ) {
		return false;
	}
	MLOGANA(DBG,this) << " OK, Bus created with parameters= " << parameters;
	return true;
}


/**
 * decode the name, parameter and return the port to the configured module
 */
int AnaCanScan::configureCanBoard(const string name,const string parameters)
{
	MLOGANA(DBG, this) << "(user supplied) parameters= " << parameters;
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

	// handle up to 5 parameter, assume defaults if needed
	m_baudRate = 125000L;

	if (strcmp(parameters.c_str(), "Unspecified") != 0) {

		MLOGANA(TRC, this) << "m_CanParameters.m_iNumberOfDetectedParameters" << m_CanParameters.m_iNumberOfDetectedParameters;

		if ( m_CanParameters.m_iNumberOfDetectedParameters >= 1 )	{
			m_baudRate = m_CanParameters.m_lBaudRate;

			MLOGANA(TRC, this) << "m_lBaudRate= " << m_CanParameters.m_lBaudRate;
			MLOGANA(TRC, this) << "m_iOperationMode= " << m_CanParameters.m_iOperationMode;
			MLOGANA(TRC, this) << "m_iTermination= " << m_CanParameters.m_iTermination;
			MLOGANA(TRC, this) << "m_iHighSpeed= " << m_CanParameters.m_iHighSpeed;
			MLOGANA(TRC, this) << "m_iTimeStamp= " << m_CanParameters.m_iTimeStamp;

			// any other parameters are already set, either to 0 by init,
			// or by decoding. They are always used.
		} else {
			MLOGANA(ERR, this) << "Error while parsing parameters: this syntax is incorrect: [" << parameters << "]";
			MLOGANA(ERR, this) << "you need up to 5 numbers separated by whitespaces, i.e. \"125000 0 0 0 0\" \"p0 p1 p2 p3 p4\"";
			MLOGANA(ERR, this) << "  p0 = baud rate, 125000 or whatever the module supports";
			MLOGANA(ERR, this) << "  p1 = operation mode";
			MLOGANA(ERR, this) << "  p2 = termination";
			MLOGANA(ERR, this) << "  p3 = high speed";
			MLOGANA(ERR, this) << "  p4 = time stamp";
			return -1;
		}
	} else	{
		MLOGANA(INF, this) << "Unspecified parameters, default values will be used.";
	}
	return openCanPort();
}

/**
 *
 * Obtains a Anagate canport and opens it.
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

	// check if USB-CANmodul already is initialized
	if (isCanHandleInUse(m_canPortNumber)) {
		//if it is, we get the handle
		canModuleHandle = getCanHandleFromPort(m_canPortNumber);
	} else {
		//Otherwise we create it.
		MLOGANA(DBG, this) << "Will call CANOpenDevice with parameters m_canHandleNumber:[" << m_canPortNumber << "], m_canIPAddress:[" << m_canIPAddress << "]";
		anaCallReturn = CANOpenDevice(&canModuleHandle, FALSE, TRUE, m_canPortNumber, m_canIPAddress.c_str(),m_timeout);
		if (anaCallReturn != 0) {
			// fill out initialisation struct
			MLOGANA(ERR,this) << "Error in CANOpenDevice, return code = [" << anaCallReturn << "]";
			return -1;
		}
	}

	setCanHandleInUse(m_canPortNumber,true);

	// initialize CAN interface

	MLOGANA(TRC,this) << "calling CANSetGlobals with m_lBaudRate= "
			<< m_CanParameters.m_lBaudRate
			<< " m_iOperationMode= " << m_CanParameters.m_iOperationMode
			<< " m_iTermination= " << m_CanParameters.m_iTermination
			<< " m_iHighSpeed= " << m_CanParameters.m_iHighSpeed
			<< " m_iTimeStamp= " << m_CanParameters.m_iTimeStamp;

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
	anaCallReturn = CANSetCallbackEx(canModuleHandle, InternalCallback);
	if (anaCallReturn != 0) {
		MLOGANA(ERR,this) << "Error in CANSetCallbackEx, return code = [" << anaCallReturn << "]";
		return -1;
	} else {
		MLOGANA(TRC,this) << "OK CANSetCallbackEx, return code = [" << anaCallReturn << "]";
	}

	setCanHandleOfPort(m_canPortNumber, canModuleHandle);
	g_AnaCanScanPointerMap[canModuleHandle] = this;

	// We associate in the global map the handle with the instance of the AnaCanScan object,
	// so it can later be identified by the callback InternalCallback in a speedy way
	m_UcanHandle = canModuleHandle;

	MLOGANA(TRC,this) << "handles: ";
	AnaCanScan::objectMapSize();
	return 0;
}

bool AnaCanScan::sendErrorCode(AnaInt32 status)
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

		if (!errorCodeToString(status, errorMessage))
			canMessageError(status, errorMessage, ftTimeStamp);
	}
	return true;
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
	MLOGANA(DBG,this) << "Sending message: [" << ( message == 0  ? "" : (const char *) message) << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";
	AnaInt32 anaCallReturn;
	unsigned char *messageToBeSent[8];
	AnaInt32 flags = 0x0;
	if (rtr) {
		flags = 2; // Bit 1: If set, the telegram is marked as remote frame.
	}
	int  messageLengthToBeProcessed;

	//If there is more than 8 characters to process, we process 8 of them in this iteration of the loop
	/// todo anyway extended can messages are not treated here yet
	if (len > 8) {
		messageLengthToBeProcessed = 8;
		MLOGANA(DBG, this) << "The length is more than 8 bytes, ignoring overhead, no extended CAN msg yet " << len;
	} else {
		messageLengthToBeProcessed = len;
	}
	// MLOG(TRC, this) << "Going to memcopy " << messageToBeSent << ";" << message << ";" << messageLengthToBeProcessed;
	memcpy(messageToBeSent, message, messageLengthToBeProcessed);
	MLOGANA(TRC, this) << " m_canPortNumber= " << m_canPortNumber
			<< " cobID= " << cobID
			<< " length = " << messageLengthToBeProcessed
			<< " flags= " << flags << " m_UcanHandle= " << m_UcanHandle;
	anaCallReturn = CANWrite(m_UcanHandle, cobID, reinterpret_cast<char*>(messageToBeSent), messageLengthToBeProcessed, flags);
	if (anaCallReturn != 0) {
		MLOGANA(ERR, this) << "There was a problem when sending a message with CANWrite: 0x" << hex << anaCallReturn << dec;
		m_canCloseDevice = false;

		/**a sending on one port failed miserably.
		 * we are pessimistic and assume that the whole module (=ip) with all ports has lost
		 * power. Therefore we try to reconnect all used ports of this ip.
		 * This is a bit brute, but it will work.
		 * Nevertheless we should limit our efforts to this module=ip only and not reconnect all other modules
		 * managed by this task.
		 * we also avoid reconnecting all ports of an ip several times (for each failed send on a port)
		 */
		AnaInt32 anaCallReturn0 = AnaCanScan::reconnectAllPorts( m_canIPAddress );
		while ( anaCallReturn0 < 0 ){
			MLOGANA(WRN, this) << "failed to reconnect all module CAN ports once, keep on trying to reconnect. ip= " << m_canIPAddress;
			anaCallReturn0 = AnaCanScan::reconnectAllPorts( m_canIPAddress );
		}
		objectMapSize();
	} else {
		m_statistics.onTransmit(messageLengthToBeProcessed);
	}
	return sendErrorCode(anaCallReturn);
}

/* static */ void AnaCanScan::objectMapSize(){
	uint32_t size = g_AnaCanScanPointerMap.size();
	LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate  ) << __FUNCTION__ << " RECEIVE obj. map size= " << size << " : ";
	for (std::map<AnaInt32, AnaCanScan*>::iterator it=g_AnaCanScanPointerMap.begin(); it!=g_AnaCanScanPointerMap.end(); it++){
		LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate ) << __FUNCTION__ << " obj. map "
				<< " key= " << it->first
				<< " ip= " << it->second->ipAdress()
				<< " CAN port= " << it->second->canPortNumber()
				<< " handle= " << it->second->handle()
				<< " (used?= " << it->second->isCanHandleInUse(it->second->handle()) << ")"
				<< " ptr= 0x" << hex << (unsigned long) (it->second) << dec;

	}
}

/**
 * reconnects all ports of a given ip adress=one module. We have one object per CAN port,
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

	// protect against several calls on the same ip.
	// We need a mutex to force serialize this.
	{
		anagateReconnectMutex.lock();
		if ( AnaCanScan::isIpReconnectInProgress( ip ) ) {
			LOG(Log::WRN, AnaCanScan::s_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
					<< " is already in progress, skipping.";

			int us = 10000000;
			boost::this_thread::sleep(boost::posix_time::microseconds( us ));
			anagateReconnectMutex.unlock();

			return(1);
		}
		AnaCanScan::setIpReconnectInProgress( ip, true );
		LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
				<< " is now in progress.";
		anagateReconnectMutex.unlock();
	}

	int ret = 0;
	AnaInt32 anaRet = 0;

	// re-open the can port and get a new handle, but do NOT yet put the new object (this, again) into
	// the global map. Keep the reception disconnected still.
	int nbCANportsOnModule = 0;
	bool reconnectFailed = false;
	for (std::map<AnaInt32, AnaCanScan*>::iterator it=g_AnaCanScanPointerMap.begin(); it!=g_AnaCanScanPointerMap.end(); it++){
		if ( ip == it->second->ipAdress() ){
			LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate ) << __FUNCTION__
					<< " key= " << it->first
					<< " found ip= " << ip
					<< " for CAN port= " << it->second->canPortNumber()
					<< " reconnecting...";

			ret = it->second->reconnect();
			if ( ret ){
				LOG(Log::WRN, AnaCanScan::s_logItHandleAnagate) << __FUNCTION__
						<< " key= " << it->first
						<< " found ip= " << ip
						<< " for CAN port= " << it->second->canPortNumber()
						<< " failed to reconnect";
				reconnectFailed = true;
			}
			nbCANportsOnModule++;
		}
	}
	LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate) << " CAN bridge at ip= " << ip << " uses  nbCANportsOnModule= " << nbCANportsOnModule;

	if ( reconnectFailed ) {
		LOG(Log::WRN, AnaCanScan::s_logItHandleAnagate ) << " Problem reconnecting CAN ports for ip= " << ip
				<< " last ret= " << ret << ". Just abandoning and trying again in 10 secs, module might not be ready yet.";
		int us = 10000000;
		boost::this_thread::sleep(boost::posix_time::microseconds( us ));

		AnaCanScan::setIpReconnectInProgress( ip, false );
		LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
				<< " cancel.";
		return(-1);
	}

	// register the new handler with the obj. map and connect the reception handler. Cleanup the stale handlers
	std::map<AnaInt32, AnaCanScan*> lmap = g_AnaCanScanPointerMap; // use a local copy of the map, in order
	// not to change the map we are iterating on

	LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
			<< " receive handler map map before reconnect for ip= " << ip;
	AnaCanScan::objectMapSize();

	for (std::map<AnaInt32, AnaCanScan*>::iterator it=lmap.begin(); it!=lmap.end(); it++){
		if ( ip == it->second->ipAdress() ){
			anaRet = it->second->connectReceptionHandler();
			if ( anaRet != 0 ){
				LOG(Log::ERR, AnaCanScan::s_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
						<< " failed to reconnect reception handler for ip= " << ip
						<< " handle= " << it->second->handle()
						<< " port= " << it->second->canPortNumber()
						<< " anaRet= " << anaRet;
			} else {
				LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate ) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
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
				LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
						<< " erasing stale handler " << it->first
						<< " for object handle= " << it->second->handle() << " from obj. map";
				g_AnaCanScanPointerMap.erase( it->first );
			}
			AnaCanScan::setIpReconnectInProgress( ip, false ); // all done, may fail another time
			LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate ) << "reconnecting all ports for ip= " << ip
					<< " is done and OK.";
		}
	}
	LOG(Log::TRC, AnaCanScan::s_logItHandleAnagate) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
			<< " receive handler map after reconnect for ip= " << ip;
	AnaCanScan::objectMapSize();

	// be easy on the switch, can maybe suppress this later
	int us = 100000;
	boost::this_thread::sleep(boost::posix_time::microseconds( us ));
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
	g_AnaCanScanPointerMap[ m_UcanHandle ] = this;
	setCanHandleInUse( m_UcanHandle, true );

	MLOGANA(DBG,this) << "RECEIVE handler looks good, handle= " << m_UcanHandle
			<< " CAN port= " << m_canPortNumber
			<< " ip= " << m_canIPAddress << " Congratulations.";
	return( anaCallReturn );
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
	int us = 100000;

	MLOGANA(WRN, this) << "try to reconnect ip= " << m_canIPAddress
			<< " m_canPortNumber= " << m_canPortNumber
			<< " m_UcanHandle= " << m_UcanHandle;

	int state = CANDeviceConnectState( m_UcanHandle );
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
		setCanHandleInUse( m_UcanHandle, false );

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

		setCanHandleInUse( m_canPortNumber, true );
		setCanHandleOfPort( m_canPortNumber, canModuleHandle );

		m_UcanHandle = canModuleHandle;
		m_canCloseDevice = false;
		break;
	}
	}
	boost::this_thread::sleep(boost::posix_time::microseconds( us ));
	return( 0 ); // OK
}

/**
 * set a connection specific timeout value.
 * Global timeout is 6000, set by CanOpenDevice
 * In order to make this specific timeout apply we have to reconnect
 */
void AnaCanScan::setConnectWaitTime( int timeout_ms ){
	m_timeout = timeout_ms;
	MLOGANA(WRN,this) << "reconnect CANOpenDevice m_UcanHandle= " << m_UcanHandle << " to apply new timeout= " << m_timeout;
	reconnect();
}

/**
 * starts explicitly the anagate keepalive mechanism
 * \todo not sure we need this actually
 */
void AnaCanScan::startAlive( int aliveTime_sec ){
	AnaInt32 anaCallReturn = CANStartAlive( m_UcanHandle, aliveTime_sec );
	if ( anaCallReturn != 0 ){
		MLOGANA(ERR,this) << "could not start alive mechanism, error=  0x" << hex << anaCallReturn << dec;
	} else {
		MLOGANA(DBG,this) << "started alive mechanism on handle= " << m_UcanHandle;
	}
}

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
	return sendErrorCode(anaCallReturn);
}

/**
 * Provides textual representation of an error code.
 * error return from module
 * \todo: Fix AnaCanScan::errorCodeToString method, this doesn't do anything!!
 */
bool AnaCanScan::errorCodeToString(long error, char message[])
{
	char tmp[300] = "Error";
	// canGetErrorText((canStatus)error, tmp, sizeof(tmp));
	// *message = new char[strlen(tmp)+1];
	//    message[0] = 0;
	strcpy(message,tmp);
	return true;
}

void AnaCanScan::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_baudRate);
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}
