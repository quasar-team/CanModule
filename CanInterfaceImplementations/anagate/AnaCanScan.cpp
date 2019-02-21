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


/* static */ bool AnaCanScan::s_isCanHandleInUseArray[256];
/* static */ AnaInt32 AnaCanScan::s_canHandleArray[256];

/** global map of connection-object-pointers: the map-key is the handle. Since handles are allocated by the OS
 * the keys are getting changed as well, so that we do not keep the stale keys(=handles) in the map at all.
 * This makes the InternalCallback code straightforward and speedy, because we can get to the object by just
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
		m_timeout ( 6000 )
{
	m_statistics.beginNewRun();
}

AnaCanScan::~AnaCanScan()
{
	MLOG(DBG,this) << "Closing down Anagate Can Scan component";
	//Shut down can scan thread
	CANSetCallback(m_UcanHandle, 0);
	CANCloseDevice(m_UcanHandle);
	MLOG(DBG,this) << "Anagate Can Scan component closed successfully";
}


/**
 * creates and returns anagate can access object
 */
extern "C" DLLEXPORTFLAG CCanAccess *getCanBusAccess()
{
	LOG(Log::TRC) << __FUNCTION__ << " " __FILE__ << " " << __LINE__;
	CCanAccess *canAccess = new AnaCanScan;
	return canAccess;
}


/**
 * call back to catch incoming CAN messages for reading
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

	//	canMsgCopy.c_time.tv_sec = nSeconds;
	//	canMsgCopy.c_time.tv_usec = nMicroseconds;
	canMsgCopy.c_time = convertTimepointToTimeval(currentTimeTimeval());

	g_AnaCanScanPointerMap[hHandle]->callbackOnRecieve(canMsgCopy);
	g_AnaCanScanPointerMap[hHandle]->statisticsOnRecieve( nBufferLen );
}



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
 * name = bus name = 3 parameters separated by ":" line "n0:n1:n2"
 * 		n0 = "an" for anagate
 * 		n1 = port number on the module, 0=A, 1=B, etc etc
 * 		n2 = ip number
 * 		ex.: "an:1:137.138.12.99" speaks to port B (=1) on anagate module at the ip
 *
 * 	parameters = up to 6 parameters separated by whitespaces : "p0 p1 p2 p3 p4 p5" in THAT order, positive integers
 * 		see module documentation for the exact meaning
 * 	    p0 = baud rate, 125000 or whatever the module supports
 * 	    p1 = operation mode
 * 	    p2 = termination
 * 	    p3 = high speed
 * 	    p4 = time stamp
 * 	    p5 = sync mode
 * i.e. "250000 0 1 0 0 1"
 */
bool AnaCanScan::createBus(const string name,const string parameters)
{	
	m_busName = name;
	m_busParameters = parameters;

	MLOG(DBG, this) << " name= " << name << " parameters= " << parameters;
	m_sBusName = name;
	int returnCode = configureCanBoard(name, parameters);
	if ( returnCode < 0 ) {
		return false;
	}
	MLOG(DBG,this) << " OK Bus name= " << name << " created with parameters= " << parameters;
	return true;
}


/**
 * decode the name, parameter and return the port to the configured module
 */
int AnaCanScan::configureCanBoard(const string name,const string parameters)
{
	MLOG(DBG, this) << " name= " << name << " parameters= " << parameters;

	vector<string> stringVector;
	stringVector = parseNameAndParameters(name, parameters);

	// we should decode 3 elements from this:0="an" for anagate library, 1=handle number, 2=ip number
	// like  "an:0:128.141.159.194"
	if ( stringVector.size() != 3 ){
		MLOG(ERR, this) << " need exactly 3 elements delimited by \":\" like \"an:0:128.141.159.194\", got "
				<< stringVector.size();
		for ( unsigned i = 0; i < stringVector.size(); i++ ){
			MLOG(ERR, this) << " stringVector[" << i << "]= " << stringVector[ i ];
		}
		return(-1);
	} else {
		for ( unsigned i = 0; i < stringVector.size(); i++ ){
			MLOG(TRC, this) << " OK stringVector[" << i << "]= " << stringVector[ i ];
		}
		m_canPortNumber = atoi(stringVector[1].c_str());
		m_canIPAddress = (char *) stringVector[2].c_str();
	}

	// handle up to 6 parameter, assume defaults if needed
	long baudRate_default = 125000;

	if (strcmp(parameters.c_str(), "Unspecified") != 0) {
		if ( m_CanParameters.m_iNumberOfDetectedParameters >= 1 )	{
			m_baudRate = m_CanParameters.m_lBaudRate;
		} else {
			MLOG(ERR, this) << "Error while parsing parameters: this syntax is incorrect: [" << parameters << "]";
			MLOG(ERR, this) << "you need 6 numbers separated by whitespaces, i.e. \"125000 0 0 0 0 0\" \"p0 p1 p2 p3 p4 p5\"";
			MLOG(ERR, this) << "  p0 = baud rate, 125000 or whatever the module supports";
			MLOG(ERR, this) << "  p1 = operation mode";
			MLOG(ERR, this) << "  p2 = termination";
			MLOG(ERR, this) << "  p3 = high speed";
			MLOG(ERR, this) << "  p4 = time stamp";
			MLOG(ERR, this) << "  p5 = sync mode";
			return -1;
		}
		// m_baudRate = baudRate_default; // set default nevertheless
	} else	{
		m_baudRate = baudRate_default;
		MLOG(INF, this) << "Unspecified parameters, default values will be used.";
	}
	/*anaCallReturn = CANSetGlobals(canModuleHandle, m_CanParameters.m_lBaudRate, m_CanParameters.m_iOperationMode,
			m_CanParameters.m_iTermination, m_CanParameters.m_iHighSpeed, m_CanParameters.m_iTimeStamp);
	 */
	return openCanPort();
}

/**
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
		MLOG(DBG, this) << "Will call CANOpenDevice with parameters m_canHandleNumber:[" << m_canPortNumber << "], m_canIPAddress:[" << m_canIPAddress << "]";
		anaCallReturn = CANOpenDevice(&canModuleHandle, FALSE, TRUE, m_canPortNumber, m_canIPAddress.c_str(),m_timeout);
		if (anaCallReturn != 0) {
			// fill out initialisation struct
			MLOG(ERR,this) << "Error in CANOpenDevice, return code = [" << anaCallReturn << "]";
			return -1;
		}
	}

	setCanHandleInUse(m_canPortNumber,true);

	// initialize CAN interface
	anaCallReturn = CANSetGlobals(canModuleHandle, m_CanParameters.m_lBaudRate, m_CanParameters.m_iOperationMode,
			m_CanParameters.m_iTermination, m_CanParameters.m_iHighSpeed, m_CanParameters.m_iTimeStamp);
	switch ( anaCallReturn ){
	case 0:{ break; }
	case 0x30000: {
		MLOG(ERR,this) << "Connection to TCP/IP partner can't be established or is disconnected. Lost TCP/IP: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	case 0x40000: {
		MLOG(ERR,this) << "No  answer  was  received  from	TCP/IP partner within the defined timeout. Lost TCP/IP: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	case 0x900000: {
		MLOG(ERR,this) << "Invalid device handle. Lost TCP/IP: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	default : {
		MLOG(ERR,this) << "Other Error in CANSetGlobals: 0x" << hex << anaCallReturn << dec;
		return -1;
	}
	}
	// set handler for managing incoming messages
	anaCallReturn = CANSetCallbackEx(canModuleHandle, InternalCallback);
	if (anaCallReturn != 0) {
		MLOG(ERR,this) << "Error in CANSetCallbackEx, return code = [" << anaCallReturn << "]";
		return -1;
	} else {
		MLOG(TRC,this) << "OK CANSetCallbackEx, return code = [" << anaCallReturn << "]";
	}

	setCanHandleOfPort(m_canPortNumber, canModuleHandle);
	g_AnaCanScanPointerMap[canModuleHandle] = this;

	// We associate in the global map the handle with the instance of the AnaCanScan object,
	// so it can later be identified by the callback InternalCallback in a speedy way
	m_UcanHandle = canModuleHandle;
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
 * send a CAN message frame (8 byte)
 */
bool AnaCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	MLOG(DBG,this) << "Sending message: [" << message << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";
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
		MLOG(DBG, this) << "The length is more than 8 bytes, ignoring overhead, no extended CAN msg yet " << len;
	} else {
		messageLengthToBeProcessed = len;
	}
	// MLOG(TRC, this) << "Going to memcopy " << messageToBeSent << ";" << message << ";" << messageLengthToBeProcessed;
	memcpy(messageToBeSent, message, messageLengthToBeProcessed);
	//MLOG(TRC, this) << " m_canPortNumber= " << m_canPortNumber
	//		<< " cobID= " << cobID
	//		<< " length = " << messageLengthToBeProcessed
	//		<< " flags= " << flags << " m_UcanHandle= " << m_UcanHandle;
	anaCallReturn = CANWrite(m_UcanHandle, cobID, reinterpret_cast<char*>(messageToBeSent), messageLengthToBeProcessed, flags);
	if (anaCallReturn != 0) {
		MLOG(ERR, this) << "There was a problem when sending a message with CANWrite: 0x" << hex << anaCallReturn << dec;
		m_canCloseDevice = false;

		/**a sending on one port failed miserably.
		 * we are pessimistic and assume that the whole module (=ip) with all ports has lost
		 * power. Therefore we try to reconnect all ports of this ip.
		 * This is a bit brute, but it will work.
		 * Nevertheless we should limit our efforts to this module=ip only and not reconnect all other modules
		 * managed by this task.
		 */
		AnaInt32 anaCallReturn0 = AnaCanScan::reconnectAllPorts( m_canIPAddress );
		while ( anaCallReturn0 ){
			MLOG(WRN, this) << "failed to reconnect all module CAN ports once, keep on trying to reconnect. ip= " << m_canIPAddress;
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
	LOG(Log::TRC) << __FUNCTION__ << " " __FILE__ << " " << __LINE__ << " RECEIVE obj. map size= " << size << " : ";
	for (std::map<AnaInt32, AnaCanScan*>::iterator it=g_AnaCanScanPointerMap.begin(); it!=g_AnaCanScanPointerMap.end(); it++){
		LOG(Log::TRC) << __FUNCTION__ << " " __FILE__ << " " << __LINE__ << " obj. map "
				<< " key= " << it->first
				<< " ip= " << it->second->ipAdress()
				<< " CAN port= " << it->second->canPortNumber()
				<< " handle= " << it->second->handle()
				<< " (used?= " << it->second->isCanHandleInUse(it->second->handle()) << ")";
	}
}

/**
 * reconnects all ports of a given ip adress=one module. We have one object per CAN port,
 * so i.e. for an anagate duo (ports A and B) we have two objects with the same IP but
 * with port 0 and 1.So we will have to scan over all objects there because we do not know
 * how many CANports a given anagate bridge has.
 * returns:
 * 0 = OK
 * -1 = cant open / reconnect CAN ports
 */
/* static */ AnaInt32 AnaCanScan::reconnectAllPorts( string ip ){
	int ret = 0;
	AnaInt32 anaRet = 0;

	// re-open the can port and get a new handle, but do NOT yet put the new object (this, again) into
	// the global map. Keep the reception disconnected still.
	int nbCANportsOnModule = 0;
	for (std::map<AnaInt32, AnaCanScan*>::iterator it=g_AnaCanScanPointerMap.begin(); it!=g_AnaCanScanPointerMap.end(); it++){
		if ( ip == it->second->ipAdress() ){
			LOG(Log::TRC) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
					<< " key= " << it->first
					<< " found ip= " << ip
					<< " for CAN port= " << it->second->canPortNumber()
					<< " reconnecting...";
			// todo: we should not add up the return calls of all ports, that is confusing
			ret += it->second->reconnect();
			nbCANportsOnModule++;
		}
	}
	LOG(Log::TRC) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
			<< " CAN bridge at ip= " << ip << " has  nbCANportsOnModule= " << nbCANportsOnModule;

	if ( ret ) {
		LOG(Log::INF) << __FUNCTION__ << " " __FILE__ << " " << __LINE__ << " Problem reconnecting CAN ports for ip= " << ip
				<< " ret= " << ret << ". Just abandoning and trying again in 10 secs.";
		int us = 10000000;
		boost::this_thread::sleep(boost::posix_time::microseconds( us ));
		return(-1);
	}

	// register the new handler with the obj. map and connect the reception handler. Cleanup the stale handlers
	std::map<AnaInt32, AnaCanScan*> lmap = g_AnaCanScanPointerMap; // use a local copy of the map, in order
	// not to change the map we are iterating on

	for (std::map<AnaInt32, AnaCanScan*>::iterator it=lmap.begin(); it!=lmap.end(); it++){
		if ( ip == it->second->ipAdress() ){
			anaRet = it->second->connectReceptionHandler();
			if ( anaRet != 0 ){
				LOG(Log::ERR) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
						<< " failed to reconnect reception handler for ip= " << ip
						<< " anaRet= " << anaRet;
			} else {
				LOG(Log::TRC) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
						<< " debug reconnect reception handler for ip= " << ip
						<< " handle= " << it->second->handle()
						<< " looking good= " << anaRet;
			}

			LOG(Log::TRC) << __FUNCTION__ << " " __FILE__ << " " << __LINE__
					<< " erasing stale handler " << it->first << " from obj. map";
			g_AnaCanScanPointerMap.erase( it->first );
		}
	}
	int us = 1000000;
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
	MLOG(WRN,this) << "connecting RECEIVE canModuleHandle= " << m_UcanHandle
			<< " ip= " << m_canIPAddress;
	anaCallReturn = CANSetCallbackEx(m_UcanHandle, InternalCallback);
	if (anaCallReturn != 0) {
		MLOG(ERR,this) << "Error debug in CANSetCallbackEx, return code = [" << anaCallReturn << "]"
				<< " canModuleHandle= " << m_UcanHandle
				<< " in a reconnect! Can't fix this. Restart program and buy better hardware please.";
		// that is very schlecht, need a good idea (~check keepalive and sending fake messages)
		// to detect and possibly recuperate from that. Does this case happen actually?
	}
	g_AnaCanScanPointerMap[ m_UcanHandle ] = this;
	setCanHandleInUse( m_UcanHandle, true );
	MLOG(INF,this) << "RECEIVE handler looks good, handle= " << m_UcanHandle
			<< " CAN port= " << m_canPortNumber
			<< " ip= " << m_canIPAddress << " Congratulations.";
	return( anaCallReturn );
}


/**
 * we try to reconnect one port
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

	MLOG(WRN, this) << "try to reconnect ip= " << m_canIPAddress
			<< " m_canPortNumber= " << m_canPortNumber
			<< " m_UcanHandle= " << m_UcanHandle;

	int state = CANDeviceConnectState( m_UcanHandle );
	MLOG(TRC, this) << "CANDeviceConnectState: device connect state= 0x" << hex << state << dec;
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
		MLOG(INF,this) << "device is in state connecting, don't try to reconnect for now.";
		break;
	}

	case 3: {
		MLOG(INF,this) << "device is connecting, don't try to reconnect, just skip.";
		break;
	}
	default:
	case 1:
	case 4:
	case 5:{
		if ( !m_canCloseDevice ) {
			anaCallReturn = CANCloseDevice( m_UcanHandle );
			MLOG(WRN, this) << "closed device m_UcanHandle= " << m_UcanHandle
					<< " anaCallReturn= 0x" << hex << anaCallReturn << dec;
			if ( anaCallReturn != 0 ) {
				MLOG(ERR, this) << "could not close device m_UcanHandle= " << m_UcanHandle
						<< " anaCallReturn= 0x" << hex << anaCallReturn << dec;
				return(-3);
			}
			m_canCloseDevice = true;
			MLOG(TRC, this) << "device is closed. stale handle= " << m_UcanHandle;
		} else {
			MLOG(WRN, this) << "device is already closed, try reopen. stale handle= " << m_UcanHandle;
		}
		setCanHandleInUse( m_UcanHandle, false );

		MLOG(TRC,this) << "try CANOpenDevice m_canPortNumber= " << m_canPortNumber
				<< " m_UcanHandle= " << m_UcanHandle
				<< " ip= " << m_canIPAddress << " timeout= " << m_timeout;
		anaCallReturn = CANOpenDevice(&canModuleHandle, FALSE, TRUE, m_canPortNumber, m_canIPAddress.c_str(), m_timeout);
		if (anaCallReturn != 0) {
			MLOG(ERR,this) << "CANOpenDevice failed: 0x" << hex << anaCallReturn << dec;
			m_canCloseDevice = false;
			return(-1);
		}
		MLOG(INF,this) << "CANOpenDevice m_canPortNumber= " << m_canPortNumber
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
	MLOG(WRN,this) << "reconnect CANOpenDevice m_UcanHandle= " << m_UcanHandle << " to apply new timeout= " << m_timeout;
	reconnect();
}

/**
 * starts explicitly the anagate keepalive mechanism
 */
void AnaCanScan::startAlive( int aliveTime_sec ){
	AnaInt32 anaCallReturn = CANStartAlive( m_UcanHandle, aliveTime_sec );
	if ( anaCallReturn != 0 ){
		MLOG(ERR,this) << "could not start alive mechanism, error=  0x" << hex << anaCallReturn << dec;
	} else {
		MLOG(INF,this) << "started alive mechanism on handle= " << m_UcanHandle;
	}
}

bool AnaCanScan::sendMessage(CanMessage *canm)
{
	return sendMessage(short(canm->c_id), canm->c_dlc, canm->c_data, canm->c_rtr);
}

bool AnaCanScan::sendRemoteRequest(short cobID)
{
	AnaInt32 anaCallReturn;
	AnaInt32 flags = 2;// Bit 1: If set, the telegram is marked as remote frame.

	anaCallReturn = CANWrite(m_UcanHandle,cobID,NULL, 0, flags);
	if (anaCallReturn != 0) {
		MLOG(ERR,this) << "Error: There was a problem when sending a message with CANWrite";
	}
	return sendErrorCode(anaCallReturn);
}

/**
 * error return from module
//TODO: Fix AnaCanScan::errorCodeToString method, this doesn't do anything!!
 *
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
