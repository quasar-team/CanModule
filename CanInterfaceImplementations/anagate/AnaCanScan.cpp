#include "AnaCanScan.h"

//////////////////////////////////////////////////////////////////////
// AnaCanScan.cpp: implementation of the AnaCanScan class.
//////////////////////////////////////////////////////////////////////

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

//! The macro below is applicable only to this translation unit

using namespace CanModule;
using namespace std;
//using namespace std::chrono;

bool AnaCanScan::s_isCanHandleInUseArray[256];
AnaInt32 AnaCanScan::s_canHandleArray[256];
std::map<AnaInt32, AnaCanScan*> g_AnaCanScanPointerMap;

extern "C" DLLEXPORTFLAG CCanAccess *getCanBusAccess()
{
	CCanAccess *canAccess;
	canAccess = new AnaCanScan;
	return canAccess;
}

AnaInt32 g_timeout = 6000;				// connect_wait time

//call back to catch incoming CAN messages
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

AnaCanScan::AnaCanScan():
m_canHandleNumber(0),
m_baudRate(0)
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


void AnaCanScan::statisticsOnRecieve(int bytes)
{
	m_statistics.onReceive( bytes );
}

void AnaCanScan::callbackOnRecieve(CanMessage& msg)
{
	canMessageCame(msg);
}

bool AnaCanScan::createBus(const string name,const string parameters)
{	
	m_sBusName = name;
	int returnCode = configureCanBoard(name, parameters);
	if (returnCode < 0)
	{
		return false;
	}

	MLOG(DBG,this) << "Bus [" << name << "] created with parameters [" << parameters << "]";
	return true;
}

int AnaCanScan::configureCanBoard(const string name,const string parameters)
{
	//Default BaudRate
	long baudRate = 125000;

	vector<string> stringVector;
	stringVector = parcerNameAndPar(name, parameters);
	m_canHandleNumber = atoi(stringVector[1].c_str());
	m_canIPAddress = stringVector[2].c_str();

	MLOG(INF, this) << "m_canHandleNumber:[" << m_canHandleNumber << "], stringVector[" << stringVector[0] << "," << stringVector[1] << "," << stringVector[2] << "]";

	if (strcmp(parameters.c_str(), "Unspecified") != 0) {

		/* Set baud rate to 125 Kbits/second.  */
		if (m_CanParameters.m_iNumberOfDetectedParameters >= 1)
		{
			m_baudRate = m_CanParameters.m_lBaudRate;
		}
		else
		{
			MLOG(ERR, this) << "Error while parsing parameters: this syntax is incorrect: [" << parameters << "]";
			return -1;
		}
		m_baudRate = baudRate;
	}
	else
	{
		m_baudRate = baudRate;
		MLOG(DBG, this) << "Unspecified parameters, default values will be used.";
	}

	//	return openCanPort(paramOperationMode, paramTermination, paramHighSpeed, paramTimeStamp);
	return openCanPort();

}

int AnaCanScan::openCanPort()
{
	AnaInt32 anaCallReturn;
	//The Handle of the can Module
	AnaInt32 canModuleHandle;

	// check if USB-CANmodul already is initialized
	if (isCanHandleInUse(m_canHandleNumber))
	{
		//if it is, we get the handle
		canModuleHandle = getCanHandle(m_canHandleNumber);
	}
	else
	{
		//Otherwise we create it.
		MLOG(DBG, this) << "Will call CANOpenDevice with parameters m_canHandleNumber:[" << m_canHandleNumber << "], m_canIPAddress:[" << m_canIPAddress << "]";
		anaCallReturn = CANOpenDevice(&canModuleHandle, FALSE, TRUE, m_canHandleNumber, m_canIPAddress,g_timeout);
		if (anaCallReturn != 0)
		{
			// fill out initialisation struct
			MLOG(ERR,this) << "Error in CANOpenDevice, return code = [" << anaCallReturn << "]";
			return -1;
		}

	}

	setCanHandleInUse(m_canHandleNumber,true);
	// initialize CAN interface
	anaCallReturn = CANSetGlobals(canModuleHandle, m_CanParameters.m_lBaudRate, m_CanParameters.m_iOperationMode,
		m_CanParameters.m_iTermination, m_CanParameters.m_iHighSpeed, m_CanParameters.m_iTimeStamp);
	if (anaCallReturn != 0)
	{
		MLOG(ERR,this) << "Error in CANSetGlobals, return code = [" << anaCallReturn << "]";
		return -1;
	}

	// set handler for managing incoming messages
	anaCallReturn = CANSetCallbackEx(canModuleHandle, InternalCallback);
	if (anaCallReturn != 0)
	{
		MLOG(ERR,this) << "Error in CANSetCallbackEx, return code = [" << anaCallReturn << "]";
		return -1;
	}

	setCanHandle(m_canHandleNumber, canModuleHandle);
	g_AnaCanScanPointerMap[canModuleHandle] = this;
	//We associate in the global map the handle with the instance of the AnaCanScan object, so it can later be identified by the callback InternalCallback
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
bool AnaCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	//	MLOG(DBG,this) << "Sending message: [" << message << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";
	AnaInt32 anaCallReturn;
	unsigned char *messageToBeSent[8];
	AnaInt32 flags = 0x0;
	if (rtr)
	{
		flags = 2; // • Bit 1: If set, the telegram is marked as remote frame.
	}
	int  messageLengthToBeProcessed;

	if (len > 8)//If there is more than 8 characters to process, we process 8 of them in this iteration of the loop
	{
		messageLengthToBeProcessed = 8;
		MLOG(DBG, this) << "The length is more than 8 bytes" << len;
	}
	else //Otherwise if there is less than 8 characters to process, we process all of them in this iteration of the loop
	{
		messageLengthToBeProcessed = len;
	}
	MLOG(TRC, this) << "Going to memcopy " << messageToBeSent << ";" << message << ";" << messageLengthToBeProcessed;
	memcpy(messageToBeSent, message, messageLengthToBeProcessed);
	MLOG(TRC, this) << "Module Number: [" << m_canHandleNumber << "], cobID: [" << cobID << "], Message Length: [" << messageLengthToBeProcessed << "]";
	MLOG(DBG, this) << "Sending message (ANAGATE API CALL): [" << messageToBeSent << "], cobID: [" << cobID << "], Message Length: [" << messageLengthToBeProcessed << "], Flags [" << flags << "], canHandle [" << m_UcanHandle << "]";
	anaCallReturn = CANWrite(m_UcanHandle, cobID, reinterpret_cast<char*>(messageToBeSent), messageLengthToBeProcessed, flags);
	if (anaCallReturn != 0)
	{
		MLOG(ERR, this) << "Error: There was a problem when sending a message.";
	}
	else
	{
		m_statistics.onTransmit(messageLengthToBeProcessed);
	}
	return sendErrorCode(anaCallReturn);
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
	if (anaCallReturn != 0)
	{
		MLOG(ERR,this) << "Error: There was a problem when sending a message.";
	}
	return sendErrorCode(anaCallReturn);
}

bool AnaCanScan::errorCodeToString(long error, char message[])//TODO: Fix this method, this doesn't do anything!!
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
