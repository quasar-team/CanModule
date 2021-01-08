/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * STCanScap.cpp
 *
 *  Created on: Jul 21, 2011
 *  Based on work by vfilimon
 *  Rework and logging done by Piotr Nikiel <piotr@nikiel.info>
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
 */

#include "STCanScan.h"

#include <time.h>
#include <string.h>
#include "CanModuleUtils.h"

#include <LogIt.h>

//! The macro below is applicable only to this translation unit
bool STCanScan::s_isCanHandleInUseArray[256];
tUcanHandle STCanScan::s_canHandleArray[256];

#define MLOGST(LEVEL,THIS) LOG(Log::LEVEL, THIS->logItHandle()) << __FUNCTION__ << " " << " systec bus= " << THIS->getBusName() << " "

#ifdef _WIN32

#define DLLEXPORTFLAG __declspec(dllexport)

#else

#include <sys/time.h>

#define DLLEXPORTFLAG
#define WINAPI

#endif

extern "C" DLLEXPORTFLAG CCanAccess *getCanBusAccess()
{
	CCanAccess *canAccess = new STCanScan;
	return canAccess;
}

STCanScan::STCanScan():
				m_CanScanThreadShutdownFlag(true),
				m_moduleNumber(0),
				m_channelNumber(0),
				m_canHandleNumber(0),
				m_busStatus( USBCAN_SUCCESSFUL ),
				m_baudRate(0),
				m_idCanScanThread(0),
				m_logItHandleSt(0)
{
	m_statistics.setTimeSinceOpened();
	m_statistics.beginNewRun();
	m_triggerCounter = m_failedSendCounter;
}
/**
 * We create and fill initializationParameters, to pass it to openCanPort
 */
 /* static */ tUcanInitCanParam STCanScan::createInitializationParameters( unsigned int baudRate ){
	tUcanInitCanParam   initializationParameters;
	initializationParameters.m_dwSize = sizeof(initializationParameters);           // size of this struct
	initializationParameters.m_bMode = kUcanModeNormal;              // normal operation mode
	initializationParameters.m_bBTR0 = HIBYTE( baudRate );              // baudrate
	initializationParameters.m_bBTR1 = LOBYTE( baudRate );
	initializationParameters.m_bOCR = 0x1A;                         // standard output
	initializationParameters.m_dwAMR = USBCAN_AMR_ALL;               // receive all CAN messages
	initializationParameters.m_dwACR = USBCAN_ACR_ALL;
	initializationParameters.m_dwBaudrate = USBCAN_BAUDEX_USE_BTR01;
	initializationParameters.m_wNrOfRxBufferEntries = USBCAN_DEFAULT_BUFFER_ENTRIES;
	initializationParameters.m_wNrOfTxBufferEntries = USBCAN_DEFAULT_BUFFER_ENTRIES;

	return ( initializationParameters );
}


/**
 * thread to supervise port activity
 */
DWORD WINAPI STCanScan::CanScanControlThread(LPVOID pCanScan)
{
	BYTE status;
	tCanMsgStruct readCanMessage;
	STCanScan *stCanScanPointer = reinterpret_cast<STCanScan *>(pCanScan);
	MLOGST(DBG,stCanScanPointer) << "CanScanControlThread Started. m_CanScanThreadShutdownFlag = [" << stCanScanPointer->m_CanScanThreadShutdownFlag <<"]";
	while (stCanScanPointer->m_CanScanThreadShutdownFlag) {

		status = UcanReadCanMsgEx(stCanScanPointer->m_UcanHandle, (BYTE *)&stCanScanPointer->m_channelNumber, &readCanMessage, 0);
		if (status == USBCAN_SUCCESSFUL) {
			if (readCanMessage.m_bFF == USBCAN_MSG_FF_RTR)
				continue;
			CanMessage canMsgCopy;
			canMsgCopy.c_id = readCanMessage.m_dwID;
			canMsgCopy.c_dlc = readCanMessage.m_bDLC;
			canMsgCopy.c_ff = readCanMessage.m_bFF;
			for (int i = 0; i < 8; i++)
				canMsgCopy.c_data[i] = readCanMessage.m_bData[i];

			canMsgCopy.c_time = convertTimepointToTimeval(currentTimeTimeval());

			stCanScanPointer->canMessageCame(canMsgCopy);
			stCanScanPointer->m_statistics.onReceive( readCanMessage.m_bDLC );
			stCanScanPointer->m_statistics.setTimeSinceReceived();

			// we can reset the reconnectionTimeout here, since we have received a message
			stCanScanPointer->resetTimeoutOnReception();
		} else {
			if (status == USBCAN_WARN_NODATA) {

				/**
				 * lets check the timeoutOnReception reconnect condition. If it is true, all we can do is to
				 * close/open the port again since the underlying hardware is hidden by socketcan abstraction.
				 * Like his we do not have to pollute the "sendMessage" like for anagate, and that is cleaner.
				 */
				CanModule::ReconnectAutoCondition rcond = stCanScanPointer->getReconnectCondition();
				CanModule::ReconnectAction ract = stCanScanPointer->getReconnectAction();
				if ( rcond == CanModule::ReconnectAutoCondition::timeoutOnReception && stCanScanPointer->hasTimeoutOnReception()) {
					if ( ract == CanModule::ReconnectAction::singleBus ){
						MLOGST(INF, stCanScanPointer) << " reconnect condition " << (int) rcond
								<< stCanScanPointer->reconnectConditionString(rcond)
								<< " triggered action " << (int) ract
								<< stCanScanPointer->reconnectActionString(ract);
						stCanScanPointer->resetTimeoutOnReception();  // renew timeout while reconnect is in progress

						// deinit single bus and reopen
						::UcanDeinitCanEx ( stCanScanPointer->m_UcanHandle, (BYTE )stCanScanPointer->m_channelNumber );
						setCanHandleInUse( stCanScanPointer->m_moduleNumber, false);

						unsigned int br = stCanScanPointer->m_baudRate;
						int ret = stCanScanPointer->openCanPort( createInitializationParameters( br ) );

						MLOGST(TRC, stCanScanPointer) << "reconnect one CAN port  m_UcanHandle= " << stCanScanPointer->m_UcanHandle;
					} else {
						MLOGST(INF, stCanScanPointer) << "reconnect action " << (int) ract
								<< stCanScanPointer->reconnectActionString(ract)
								<< " is not implemented for systec";
					}
				}  // reconnect condition

				Sleep( 100 ); // ms

			} else {
				stCanScanPointer->sendErrorCode(status);
			}
		}
	}
	ExitThread(0);
	return 0;
}

STCanScan::~STCanScan()
{
	m_CanScanThreadShutdownFlag = false;
	DWORD result = WaitForSingleObject(m_hCanScanThread, INFINITE); 	//Shut down can scan thread
	::UcanDeinitCanEx (m_UcanHandle,(BYTE)m_channelNumber);
	MLOGST(DBG,this) << __FUNCTION__ <<" closed successfully";
}

/**
 * Method that initialises a CAN bus channel for systec@windows.
 * All following methods called on the same object will be using this initialized channel.
 *
 * @param name = 2 parameters separated by ":" like "n0:n1"
 * 		* n0 = "st" for systec@windows
 * 		* n1 = CAN port number on the module, can be prefixed with "can"
 * 		* ex.: "st:can1" speaks to port 1 on systec module at the ip
 * 		* ex.: "st:1" works as well
 *
 *
 * @param parameters one parameter: "p0", positive integers
 * 				* "Unspecified" (or empty): using defaults = "125000" // params missing
 * 				* p0: bitrate: 50000, 100000, 125000, 250000, 500000, 1000000 bit/s
 *				  i.e. "250000"
 *
 * @return was the initialisation process successful?
 *
 * ===note from Piotr===
 * in the Windows implementation of Systec (hardware component: st):
 * to define the interface naming as canX, where X is a non-negative number
 * note: the SysTec driver for windows expects addressing in terms of module
 * number and channel (e.g. 0:0 is can0, 1:0 is can2, 1:1 is can3, etc)
 * so can0 should open a SysTec interface 0:0 matching "can0" on the box,
 * can1 should open a SysTec interface 0:1 matching "can1" on the box, etc.
 *
 * returns:
 * 0 = OK, bus created
 * -1 = not ok, board init failed
 * -2 = not ok, thread creation failed
 *
 * no implemented: 1=OK, bus creation skipped since it exists already
 *
 */
int STCanScan::createBus(const string name,const string parameters)
{	
	LogItInstance* logItInstance = CCanAccess::getLogItInstance(); // actually calling instance method, not class

	if ( !LogItInstance::setInstance(logItInstance))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not set LogIt instance" << std::endl;

	if (!logItInstance->getComponentHandle(CanModule::LogItComponentName, m_logItHandleSt))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not get LogIt component handle for " << LogItComponentName << std::endl;


	MLOGST(DBG, this) << " name= " << name << " parameters= " << parameters << ", configuring CAN board";
	m_sBusName = name;
	int returnCode = configureCanBoard(name, parameters);
	if (returnCode < 0) {
		return (-1);
	}

	// After the canboard is configured and started, we start the scan control thread
	m_hCanScanThread = CreateThread(NULL, 0, CanScanControlThread, this, 0, &m_idCanScanThread);
	if (NULL == m_hCanScanThread) {
		MLOGST(ERR,this) << "creating the canScanControl thread.";
		return (-2);
	}
	MLOGST(DBG,this) <<  " Bus [" << name << "] created with parameters [" << parameters << "]";
	return (0);
}

/**
 * configures systec @ windows board
 * only G1/G2 boards
 */
int STCanScan::configureCanBoard(const string name,const string parameters)
{
	long baudRate = USBCAN_BAUD_125kBit;
	m_CanParameters.m_lBaudRate = 125000;

	vector<string> stringVector;
	stringVector = parseNameAndParameters(name, parameters);

	const char *na = stringVector[1].c_str();
	m_canHandleNumber = atoi(na);
	m_moduleNumber = m_canHandleNumber/2;
	m_channelNumber = m_canHandleNumber%2;	

	MLOGST(TRC, this) << " m_canHandleNumber:[" << m_canHandleNumber << "], m_moduleNumber:[" << m_moduleNumber << "], m_channelNumber:[" << m_channelNumber << "]";

	if (strcmp(parameters.c_str(), "Unspecified") != 0) {
		/* Set baud rate to 125 Kbits/second.  */
		if (m_CanParameters.m_iNumberOfDetectedParameters >= 1) {
			switch (m_CanParameters.m_lBaudRate)
			{
			case 50000: baudRate = USBCAN_BAUD_50kBit; break;
			case 100000: baudRate = USBCAN_BAUD_100kBit; break;
			case 125000: baudRate = USBCAN_BAUD_125kBit; break;
			case 250000: baudRate = USBCAN_BAUD_250kBit; break;
			case 500000: baudRate = USBCAN_BAUD_500kBit; break;
			case 1000000: baudRate = USBCAN_BAUD_1MBit; break;
			default: {
				baudRate = USBCAN_BAUD_125kBit;
				MLOGST(WRN, this) << "baud rate illegal, taking default 125000 [" << baudRate << "]";
			}
			}
		} else {
			MLOGST(ERR, this) << "parsing parameters: this syntax is incorrect: [" << parameters << "]";
			return -1;
		}
	} else 	{
		MLOGST(DBG, this) << "Unspecified parameters, default values will be used.";
	}
	m_baudRate = baudRate;
	m_CanParameters.m_lBaudRate = vendorBaudRate2UserBaudRate( baudRate );
	return openCanPort( createInitializationParameters( m_baudRate ) );
}

/**
 * unfortunately it is necessary to convert the vendor baudrate back to the human readable user baudrate
 * for statistics.
 * we use presently ONLY G1/G2 boards (see usbcan.h)
 */
unsigned int STCanScan::vendorBaudRate2UserBaudRate( unsigned int vb ){
	switch (vb) {
	// G1, G2 boards
	case USBCAN_BAUD_50kBit: return( 50000 ); break;
	case USBCAN_BAUD_100kBit: return( 100000 ); break;
	case USBCAN_BAUD_125kBit: return( 125000 ); break;
	case USBCAN_BAUD_250kBit: return( 250000 ); break;
	case USBCAN_BAUD_500kBit: return( 500000 ); break;
	case USBCAN_BAUD_1MBit: return( 1000000 ); break;

	// G3 boards
	// case USBCAN_BAUDEX_10kBit: return( 10000 ); break;
	// ...

	// whatnot other boards...
	default: {
		MLOGST(ERR, this) << __FUNCTION__ << " vendor baud rate illegal for G1/G2 boards [" << vb << "]";
		return(0);
	}
	} // switch
}

/**
 * Obtains a Systec canport and opens it.
 *  The name of the port and parameters should have been specified by preceding call to configureCanboard()
 *
 *  @returns less than zero in case of error, otherwise success
 */
int STCanScan::openCanPort(tUcanInitCanParam initializationParameters)
{
	BYTE systecCallReturn = USBCAN_SUCCESSFUL;
	tUcanHandle		canModuleHandle;

	// check if USB-CANmodul already is initialized
	if (isCanHandleInUse(m_moduleNumber)) {
		canModuleHandle = getCanHandle(m_moduleNumber); //if it is, we get the handle
		MLOGST(WRN,this) << "trying to open a can port which is in use, reuse handle, skipping UCanDeinitHardware";
	} else {
		//Otherwise we create it.
		MLOGST(TRC,this) << "init can port";
		systecCallReturn = ::UcanInitHardwareEx(&canModuleHandle, m_moduleNumber, 0, 0);
		if (systecCallReturn != USBCAN_SUCCESSFUL ) 	{
			MLOGST(ERR,this) << "UcanInitHardwareEx, return code = [ 0x" << hex << (int) systecCallReturn << dec << "]";
			::UcanDeinitHardware(canModuleHandle);
			return -1;
		}
	}

	setCanHandleInUse(m_moduleNumber,true);
	systecCallReturn = ::UcanInitCanEx2(canModuleHandle, m_channelNumber, &initializationParameters);
	if ( systecCallReturn != USBCAN_SUCCESSFUL )	{
		MLOGST(ERR,this) << "UcanInitCanEx2, return code = [ 0x" << hex << (int) systecCallReturn << dec << "]";
		::UcanDeinitCanEx(canModuleHandle, m_channelNumber);
		return -1;
	}
	setCanHandle(m_moduleNumber, canModuleHandle);
	m_UcanHandle = canModuleHandle;
	m_statistics.setTimeSinceOpened();
	return 0;
}

/**
 * get CAN port and USB status, code it into an int > 200 (for windows@systec).
 * use the API directly
 * table19 says for CAN status:
 * 0x0: no error
 * 0x1: tx overrun
 * 0x2: rx overrun
 * 0x4: error limit1 exceeded: warning limit
 * 0x8: error limit2 exceeded: error passive
 * 0x10: can controller is off
 * 0x40: rx buffer overrun
 * 0x80: tx buffer overrun
 * 0x400: transmit timeout, message dropped
 *
 * table20 says for USB status:
 * 0x2000: module/usb got reset because of polling failure per second
 * 0x4000: module/usb got reset because watchdog was not triggered
 */
uint32_t STCanScan::getPortStatus(){
	uint32_t stat2 = 0;
	tStatusStruct stat;
	::UcanGetStatus( m_UcanHandle, &stat );
	stat2 = stat.m_wCanStatus + stat.m_wUsbStatus;
	return( stat2 | CANMODULE_STATUS_BP_SYSTEC );
}

bool STCanScan::sendErrorCode(long status)
{
	char errorMessage[120];
	timeval ftTimeStamp; 
	if (status != m_busStatus) {

		ftTimeStamp = convertTimepointToTimeval(currentTimeTimeval());

		if (!errorCodeToString(status, errorMessage))
			canMessageError(status, errorMessage, ftTimeStamp);
		m_busStatus = status;
	}
	return true;
}

/**
 * hard reset the bridge and reconnect all ports and handlers: windows
 */
/* static */ int STCanScan::reconnectAllPorts( tUcanHandle h ){
	//MLOGST(WRN,this) << __FUNCTION__ << " hard reset not yet implemented";
	std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " hard reset not yet implemented";
#if 0
	/// have to call these two API methods
	BYTE ret0 = ::UcanDeinitHardware ( m_UcanHandle );
	if ( ret0 != USBCAN_SUCCESSFUL) {
	}

	BYTE ret1 = ::UcanInitHardwareEx(&m_UcanHandle, m_moduleNumber, 0, 0);
	if ( ret1 != USBCAN_SUCCESSFUL) {
	}

	/**
	 * and now reconnect all ports, but n order to do that we need to keep track of them globally.
	 * That is a risky thing to do under windows, and takes some careful programming.
	 * Since I am not sure this is really needed at this point I skip it for now.
	 */
#endif
	return(0);
}

/**
 * Method that sends a message trough the can bus channel. If the method createBUS
 * was not called before this, sendMessage will fail, as there is no
 * can bus channel to send a message through.
 *
 * @param cobID Identifier that will be used for the message.
 * @param len Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
 * @param message Message to be sent trough the can bus.
 * @param rtr is the message a remote transmission request?
 * @return Was the sending process successful?
 */
bool STCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	MLOGST(DBG,this) << "Sending message: [" << ( message == 0  ? "" : (const char *) message) << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";

	tCanMsgStruct canMsgToBeSent;
	BYTE Status;

	canMsgToBeSent.m_dwID = cobID;
	canMsgToBeSent.m_bDLC = len;
	canMsgToBeSent.m_bFF = 0;
	if (rtr) {
		canMsgToBeSent.m_bFF = USBCAN_MSG_FF_RTR;
	}
	int  messageLengthToBeProcessed;
	//If there is more than 8 characters to process, we process 8 of them in this iteration of the loop
	if (len > 8) {
		messageLengthToBeProcessed = 8;
		MLOGST(DBG, this) << "The length is more then 8 bytes, adjust to 8, ignore >8. len= " << len;
	} else {
		//Otherwise if there is less than 8 characters to process, we process all of them in this iteration of the loop
		messageLengthToBeProcessed = len;
		if (len < 8) {
			MLOGST(DBG, this) << "The length is less then 8 bytes, process only. len= " << len;
		}
	}
	canMsgToBeSent.m_bDLC = messageLengthToBeProcessed;
	memcpy(canMsgToBeSent.m_bData, message, messageLengthToBeProcessed);
	//	MLOG(TRC,this) << "Channel Number: [" << m_channelNumber << "], cobID: [" << canMsgToBeSent.m_dwID << "], Message Length: [" << static_cast<int>(canMsgToBeSent.m_bDLC) << "]";
	Status = UcanWriteCanMsgEx(m_UcanHandle, m_channelNumber, &canMsgToBeSent, NULL);
	if ( Status != USBCAN_SUCCESSFUL ) {
		MLOGST(ERR,this) << "There was a problem when sending a message.";

		switch( m_reconnectCondition ){
		case CanModule::ReconnectAutoCondition::sendFail: {
			MLOGST(WRN, this) << " detected a sendFail, triggerCounter= " << m_triggerCounter
					<< " failedSendCounter= " << m_failedSendCounter;
			m_triggerCounter--;
			break;
		}
		case CanModule::ReconnectAutoCondition::never:
		default:{
			m_triggerCounter = m_failedSendCounter;
			break;
		}
		}

		MLOGST(TRC, this) << " triggerCounter= " << m_triggerCounter;


		switch ( m_reconnectAction ){
		case CanModule::ReconnectAction::allBusesOnBridge: {
			if ( m_triggerCounter <= 0 ){
				MLOGST(INF, this) << " reconnect condition " << (int) m_reconnectCondition
						<< reconnectConditionString(m_reconnectCondition)
						<< " triggered action " << (int) m_reconnectAction
						<< reconnectActionString(m_reconnectAction);

				STCanScan::reconnectAllPorts( m_UcanHandle );
				m_triggerCounter = m_failedSendCounter;
			}
			break;
		}
		case CanModule::ReconnectAction::singleBus: {
			if ( m_triggerCounter <= 0 ){
				MLOGST(INF, this) << " reconnect condition " << (int) m_reconnectCondition
						<< reconnectConditionString(m_reconnectCondition)
						<< " triggered action " << (int) m_reconnectAction
						<< reconnectActionString(m_reconnectAction);
				openCanPort( createInitializationParameters( m_baudRate ));
				MLOGST(TRC, this) << "reconnect one CAN port  m_UcanHandle= " << m_UcanHandle;
				m_triggerCounter = m_failedSendCounter;
			}
			break;
		}
		default: {
			break;
		}
		}
	} else {
		m_statistics.onTransmit( canMsgToBeSent.m_bDLC );
		m_statistics.setTimeSinceTransmitted();
	}
	return sendErrorCode(Status);
}

bool STCanScan::sendRemoteRequest(short cobID)
{
	tCanMsgStruct canMsg;
	BYTE Status;
	canMsg.m_dwID = cobID;
	canMsg.m_bDLC = 0;
	canMsg.m_bFF = USBCAN_MSG_FF_RTR;	//Needed for send message rtr

	Status = UcanWriteCanMsgEx(m_UcanHandle, m_channelNumber, &canMsg, NULL);
	return sendErrorCode(Status);
}

bool STCanScan::errorCodeToString(long error, char message[])//TODO: Fix this method, this doesn't do anything!!
{
	char tmp[300] = "Error";
	// canGetErrorText((canStatus)error, tmp, sizeof(tmp));
	//	message = new char[strlen(tmp)+1];
	message[0] = 0;
	strcpy(message,tmp);
	return true;
}

void STCanScan::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_baudRate);
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}

