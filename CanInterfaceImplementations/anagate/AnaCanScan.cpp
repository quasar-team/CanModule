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

#ifdef _WIN32

#include "gettimeofday.h"

#define DLLEXPORTFLAG __declspec(dllexport)

#else

#include <sys/time.h>

#define DLLEXPORTFLAG  
#define WINAPI  

#endif

//! The macro below is applicable only to this translation unit
#define MLOG(LEVEL,THIS) LOG(Log::LEVEL) << THIS->m_canName << " "
using namespace CanModule;
using namespace std;

bool AnaCanScan::s_isCanHandleInUseArray[256];
AnaInt32 AnaCanScan::s_canHandleArray[256];
std::map<AnaInt32, AnaCanScan*> g_AnaCanScanPointerMap;

extern "C" DLLEXPORTFLAG CCanAccess *getCanbusAccess()
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
	canMsgCopy.c_id = nIdentifier;
	canMsgCopy.c_dlc = nBufferLen;
	canMsgCopy.c_ff = nFlags;
	for (int i = 0; i < nBufferLen; i++)
		canMsgCopy.c_data[i] = pcBuffer[i];
	gettimeofday(&(canMsgCopy.c_time), 0);
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
	MLOG(DBG,this) << "Closing down ST Can Scan component";
	//Shut down can scan thread
     // deinitialize CAN interface
    //::UcanDeinitCanEx (m_UcanHandle,(BYTE)m_channelNumber);
    MLOG(DBG,this) << "ST Can Scan component closed successfully";
}


void AnaCanScan::statisticsOnRecieve(int bytes)
{
	m_statistics.onReceive( bytes );
}

void AnaCanScan::callbackOnRecieve(CanMessage& msg)
{
	canMessageCame(msg);
}

bool AnaCanScan::createBUS(const char *name,const char *parameters)
{	
	int returnCode = configureCanboard(name, parameters);
	if (returnCode < 0)
	{
		return false;
	}

	MLOG(DBG,this) << "Bus [" << name << "] created with parameters [" << parameters << "]";
	return true;
}

int AnaCanScan::configureCanboard(const char *name,const char *parameters)
{
	//The different parameters
	unsigned int paramBaudRate = 0;
	unsigned int paramOperationMode = 0;
	unsigned int paramTermination = 0;
	unsigned int paramHighSpeed = 0;
	unsigned int paramTimeStamp = 0;
	//Default BaudRate
	long baudRate = 125000;
	int	numberOfDetectedParameters;

	vector<string> stringVector;
	istringstream nameSS(name);
	string temporalString;
	while (getline(nameSS, temporalString, ':')) {
		stringVector.push_back(temporalString);
	}

	m_canHandleNumber = atoi(stringVector[1].c_str());
	m_canIPAddress = stringVector[2].c_str();
	MLOG(DBG, this) << "m_canHandleNumber:[" << m_canHandleNumber << "], stringVector[" << stringVector[0] << "," << stringVector[1] << "," << stringVector[2] << "]";

	if (strcmp(parameters, "Unspecified") != 0)
	{
#ifdef _WIN32
		numberOfDetectedParameters = sscanf_s(parameters, "%d %d %d %d %d %d", &paramBaudRate, &paramOperationMode, &paramTermination, &paramHighSpeed, &paramTimeStamp);
#else
		numberOfDetectedParameters = sscanf(parameters, "%d %d %d %d %d %d", &paramBaudRate, &paramOperationMode, &paramTermination, &paramHighSpeed, &paramTimeStamp);
#endif

		/* Set baud rate to 125 Kbits/second.  */
		if (numberOfDetectedParameters >= 1)
		{
			m_baudRate = paramBaudRate;
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

	return openCanPort(paramOperationMode, paramTermination, paramHighSpeed, paramTimeStamp);
}

int AnaCanScan::openCanPort(unsigned int operationMode, unsigned int bTermination, unsigned int bHighspeed, unsigned int bTimestamp)
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
	anaCallReturn = CANSetGlobals(canModuleHandle, m_baudRate, operationMode, bTermination,bHighspeed, bTimestamp);
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
		gettimeofday(&ftTimeStamp,0);
		if (!errorCodeToString(status, errorMessage))
			canMessageError(status, errorMessage, ftTimeStamp);
	}
	return true;
}

bool AnaCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	MLOG(DBG,this) << "Sending message: [" << message << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";
	AnaInt32 anaCallReturn;
	unsigned char *unprocessedMessageBuffer = message;
	unsigned char *messageToBeSent[8];
	AnaInt32 flags = 0x0;
	if (rtr)
	{
		flags = 2; // • Bit 1: If set, the telegram is marked as remote frame.
	}
	int unprocessedMessageLength, messageLengthToBeProcessed;
	unprocessedMessageLength = len;
	do
	{
		if (unprocessedMessageLength > 8)//If there is more than 8 characters to process, we process 8 of them in this iteration of the loop
		{
			messageLengthToBeProcessed = 8;
			unprocessedMessageLength = unprocessedMessageLength - 8;
		}
		else //Otherwise if there is less than 8 characters to process, we process all of them in this iteration of the loop
		{
			messageLengthToBeProcessed = unprocessedMessageLength;
		}
		MLOG(TRC, this) << "Going to memcopy " << messageToBeSent << ";" << unprocessedMessageBuffer << ";" << messageLengthToBeProcessed;
		memcpy(messageToBeSent, unprocessedMessageBuffer, messageLengthToBeProcessed);
		MLOG(TRC,this) << "Module Number: [" << m_canHandleNumber << "], cobID: [" << cobID << "], Message Length: [" << messageLengthToBeProcessed << "]";
		MLOG(DBG, this) << "Sending message (ANAGATE API CALL): [" << messageToBeSent << "], cobID: [" << cobID << "], Message Length: [" << messageLengthToBeProcessed << "], Flags [" << flags << "], canHandle [" << m_UcanHandle << "]";
		anaCallReturn = CANWrite(m_UcanHandle, cobID, reinterpret_cast<char*>(messageToBeSent), messageLengthToBeProcessed, flags);
		if (anaCallReturn != 0)
		{
			MLOG(ERR,this) << "Error: There was a problem when sending a message.";
			break;
		}
		else
		{
			m_statistics.onTransmit( messageLengthToBeProcessed );
		}
		unprocessedMessageBuffer = unprocessedMessageBuffer + messageLengthToBeProcessed;
	}
	while (unprocessedMessageLength > 8);
	return sendErrorCode(anaCallReturn);
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
  char tmp[300];
// canGetErrorText((canStatus)error, tmp, sizeof(tmp));
// *message = new char[strlen(tmp)+1];	
    message[0] = 0;
// strcpy(*message,tmp);
  return true;
}

void AnaCanScan::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_baudRate);
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}

bool AnaCanScan::initialiseLogging(LogItInstance* remoteInstance)
{
	LOG(Log::INF) << "DLL: logging initialised";
	bool ret = Log::initializeDllLogging(remoteInstance);
	LOG(Log::INF) << "DLL: logging initialised";
	return ret;
}
