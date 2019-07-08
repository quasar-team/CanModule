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

/* static */ bool STCanScan::s_logItRegisteredSt = false;
/* static */ Log::LogComponentHandle STCanScan::s_logItHandleSt = 0;

#define MLOGST(LEVEL,THIS) LOG(Log::LEVEL, STCanScan::s_logItHandleSt) << __FUNCTION__ << " " << " systec bus= " << THIS->getBusName() << " "

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
				m_busStatus(USBCAN_SUCCESSFUL),
				m_baudRate(0),
				m_idCanScanThread(0)
{
	m_statistics.beginNewRun();
}


/**
 * thread to supervise port activity
 */
DWORD WINAPI STCanScan::CanScanControlThread(LPVOID pCanScan)
{
	BYTE status;
	tCanMsgStruct readCanMessage;
	STCanScan *canScanInstancePointer = reinterpret_cast<STCanScan *>(pCanScan);
	MLOGST(DBG,canScanInstancePointer) << "CanScanControlThread Started. m_CanScanThreadShutdownFlag = [" << canScanInstancePointer->m_CanScanThreadShutdownFlag <<"]";
	while (canScanInstancePointer->m_CanScanThreadShutdownFlag) {
		status = UcanReadCanMsgEx(canScanInstancePointer->m_UcanHandle, (BYTE *)&canScanInstancePointer->m_channelNumber, &readCanMessage, 0);
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

			canScanInstancePointer->canMessageCame(canMsgCopy);
			canScanInstancePointer->m_statistics.onReceive( readCanMessage.m_bDLC );
		} else {
			if (status == USBCAN_WARN_NODATA) {
				Sleep(100);
			} else {
				canScanInstancePointer->sendErrorCode(status);
			}
		}
	}
	ExitThread(0);
	return 0;
}

STCanScan::~STCanScan()
{
	m_CanScanThreadShutdownFlag = false;
	//Shut down can scan thread
	DWORD result = WaitForSingleObject(m_hCanScanThread, INFINITE);
	// deinitialize CAN interface
	::UcanDeinitCanEx (m_UcanHandle,(BYTE)m_channelNumber);
	MLOGST(DBG,this) << "ST Can Scan component closed successfully";
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
 */
bool STCanScan::createBus(const string name,const string parameters)
{	
	// calling base class to get the instance from there
	Log::LogComponentHandle myHandle;
	LogItInstance* logItInstance = CCanAccess::getLogItInstance(); // actually calling instance method, not class
	//std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " ptr= 0x" << logItInstance << std::endl;

	// register systec@W component for logging
	if ( !LogItInstance::setInstance(logItInstance))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not set LogIt instance" << std::endl;

	logItInstance->registerLoggingComponent( CanModule::LogItComponentName, Log::TRC );
	if (!logItInstance->getComponentHandle(CanModule::LogItComponentName, myHandle))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not get LogIt component handle for " << LogItComponentName << std::endl;

	STCanScan::s_logItHandleSt = myHandle;

	MLOGST(DBG, this) << " name= " << name << " parameters= " << parameters << ", configuring CAN board";
	m_sBusName = name;
	int returnCode = configureCanBoard(name, parameters);
	if (returnCode < 0) {
		return false;
	}

	// After the canboard is configured and started, we start the scan control thread
	m_hCanScanThread = CreateThread(NULL, 0, CanScanControlThread, this, 0, &m_idCanScanThread);
	if (NULL == m_hCanScanThread) {
		MLOGST(ERR,this) << "creating the canScanControl thread.";
		return false;
	}
	MLOGST(DBG,this) <<  " Bus [" << name << "] created with parameters [" << parameters << "]";
	return true;
}

int STCanScan::configureCanBoard(const string name,const string parameters)
{
	long baudRate = USBCAN_BAUD_125kBit;

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
			default: baudRate = m_CanParameters.m_lBaudRate;
			}
		} else {
			MLOGST(ERR, this) << "parsing parameters: this syntax is incorrect: [" << parameters << "]";
			return -1;
		}
		m_baudRate = baudRate;
	} else 	{
		m_baudRate = baudRate;
		MLOGST(DBG, this) << "Unspecified parameters, default values will be used.";
	}

	// We create and fill initializationParameters, to pass it to openCanPort
	tUcanInitCanParam   initializationParameters;
	initializationParameters.m_dwSize = sizeof(initializationParameters);           // size of this struct
	initializationParameters.m_bMode = kUcanModeNormal;              // normal operation mode
	initializationParameters.m_bBTR0 = HIBYTE(baudRate);              // baudrate
	initializationParameters.m_bBTR1 = LOBYTE(baudRate);
	initializationParameters.m_bOCR = 0x1A;                         // standard output
	initializationParameters.m_dwAMR = USBCAN_AMR_ALL;               // receive all CAN messages
	initializationParameters.m_dwACR = USBCAN_ACR_ALL;
	initializationParameters.m_dwBaudrate = USBCAN_BAUDEX_USE_BTR01;
	initializationParameters.m_wNrOfRxBufferEntries = USBCAN_DEFAULT_BUFFER_ENTRIES;
	initializationParameters.m_wNrOfTxBufferEntries = USBCAN_DEFAULT_BUFFER_ENTRIES;

	return openCanPort(initializationParameters);
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
	//The Handle of the can Module
	tUcanHandle		canModuleHandle;

	// check if USB-CANmodul already is initialized
	if (isCanHandleInUse(m_moduleNumber))
	{
		//if it is, we get the handle
		canModuleHandle = getCanHandle(m_moduleNumber);
	}
	else
	{
		//Otherwise we create it.
		systecCallReturn = ::UcanInitHardwareEx(&canModuleHandle, m_moduleNumber, 0, 0);
		if (systecCallReturn != USBCAN_SUCCESSFUL)
		{
			// fill out initialisation struct
			MLOGST(ERR,this) << "UcanInitHardwareEx, return code = [" << systecCallReturn << "]";
			::UcanDeinitHardware(canModuleHandle);
			return -1;
		}
	}

	setCanHandleInUse(m_moduleNumber,true);
	// initialize CAN interface
	systecCallReturn = ::UcanInitCanEx2(canModuleHandle, m_channelNumber, &initializationParameters);
	if (systecCallReturn != USBCAN_SUCCESSFUL)
	{
		MLOGST(ERR,this) << "UcanInitCanEx2, return code = [" << systecCallReturn << "]";
		::UcanDeinitCanEx(canModuleHandle, m_channelNumber);
		return -1;
	}
	setCanHandle(m_moduleNumber, canModuleHandle);
	m_UcanHandle = canModuleHandle;
	return 0;
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
	if (rtr)
	{
		canMsgToBeSent.m_bFF = USBCAN_MSG_FF_RTR;
	}
	int  messageLengthToBeProcessed;
	if (len > 8)//If there is more than 8 characters to process, we process 8 of them in this iteration of the loop
	{
		messageLengthToBeProcessed = 8;
		MLOGST(DBG, this) << "The length is more then 8 bytes, adjust to 8, ignore >8. len= " << len;
	}
	else  //Otherwise if there is less than 8 characters to process, we process all of them in this iteration of the loop
	{
		messageLengthToBeProcessed = len;
		if (len < 8) {
			MLOGST(DBG, this) << "The length is less then 8 bytes, process only. len= " << len;
		}
	}
	canMsgToBeSent.m_bDLC = messageLengthToBeProcessed;
	memcpy(canMsgToBeSent.m_bData, message, messageLengthToBeProcessed);
	//	MLOG(TRC,this) << "Channel Number: [" << m_channelNumber << "], cobID: [" << canMsgToBeSent.m_dwID << "], Message Length: [" << static_cast<int>(canMsgToBeSent.m_bDLC) << "]";
	Status = UcanWriteCanMsgEx(m_UcanHandle, m_channelNumber, &canMsgToBeSent, NULL);
	if (Status != USBCAN_SUCCESSFUL)
	{
		MLOGST(ERR,this) << "There was a problem when sending a message.";
	}
	else
	{
		m_statistics.onTransmit( canMsgToBeSent.m_bDLC );
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

