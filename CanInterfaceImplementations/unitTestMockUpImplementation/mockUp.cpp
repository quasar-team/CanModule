#include "mockUp.h"

//////////////////////////////////////////////////////////////////////
// mockUp.cpp: implementation of the mockUp class.
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

bool mockUp::s_isCanHandleInUseArray[256];
int mockUp::s_canHandleArray[256];
std::map<int, mockUp*> g_mockUpPointerMap;
int g_counter = 0;
extern "C" DLLEXPORTFLAG CCanAccess *getCanbusAccess()
{
	CCanAccess *canAccess;
	canAccess = new mockUp;
	return canAccess;
}

int g_timeout = 6000;				// connect_wait time

/*void UnixTimeToFileTime(time_t time, LPFILETIME fileTime)
{
    // Note that LONGLONG is a 64-bit value
    LONGLONG longLong;
	longLong = Int32x32To64(time, 10000000) + 116444736000000000;
	fileTime->dwLowDateTime = (DWORD)longLong;
	fileTime->dwHighDateTime = longLong >> 32;
}*/

//call back to catch incoming CAN messages
void WINAPI InternalCallback(int nIdentifier, const char * pcBuffer, int nBufferLen, int nFlags, int hHandle, int nSeconds, int nMicroseconds)
{

    //calculate the number of lost frames
	/*g_received = *((int*) &pcBuffer[0]); //to take the start of the buffer
	if (g_sequence != g_received) {
		g_lostFrames += (g_received - g_sequence);
		g_sequence = g_received+1;

	}
	else
		++g_sequence;*/
	//LOG(Log::DBG) << g_mockUpPointerMap[hHandle]->getBusName();
	CanMessage canMsgCopy;
	canMsgCopy.c_id = nIdentifier;
	canMsgCopy.c_dlc = nBufferLen;
	canMsgCopy.c_ff = nFlags;
	for (int i = 0; i < nBufferLen; i++)
		canMsgCopy.c_data[i] = pcBuffer[i];
	gettimeofday(&(canMsgCopy.c_time), 0);
	g_mockUpPointerMap[hHandle]->callbackOnRecieve(canMsgCopy);
	g_mockUpPointerMap[hHandle]->statisticsOnRecieve( nBufferLen );
}

mockUp::mockUp():
m_canHandleNumber(0),
m_baudRate(0)
{
	m_statistics.beginNewRun();
}

mockUp::~mockUp()
{
	MLOG(DBG,this) << "Closing down ST Can Scan component";
    MLOG(DBG,this) << "ST Can Scan component closed successfully";
}


void mockUp::statisticsOnRecieve(int bytes)
{
	m_statistics.onReceive( bytes );
}

void mockUp::callbackOnRecieve(CanMessage& msg)
{
	canMessageCame(msg);
}

bool mockUp::createBUS(const char *name,const char *parameters)
{	
	int returnCode = configureCanboard(name, parameters);
	if (returnCode < 0)
	{
		return false;
	}

	//MLOG(DBG,this) << "Bus [" << name << "] created with parameters [" << parameters << "]";
	return true;
}

int mockUp::configureCanboard(const char *name,const char *parameters)
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
	//MLOG(DBG, this) << "m_canHandleNumber:[" << m_canHandleNumber << "], stringVector[" << stringVector[0] << "," << stringVector[1] << "," << stringVector[2] << "]";
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
	m_baudRate = baudRate;

	return 0;//openCanPort(paramOperationMode, paramTermination, paramHighSpeed, paramTimeStamp);
}

int mockUp::openCanPort(unsigned int operationMode, unsigned int bTermination, unsigned int bHighspeed, unsigned int bTimestamp)
{	
	int canModuleHandle = g_counter++;
	g_mockUpPointerMap[canModuleHandle] = this;
	//We associate in the global map the handle with the instance of the mockUp object, so it can later be identified by the callback InternalCallback
	m_UcanHandle = canModuleHandle;
	return 0;
}

bool mockUp::sendErrorCode(int status)
{
	return true;
}

bool mockUp::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	MLOG(DBG,this) << "Sending message: [" << message << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";
	//tCanMsgStruct canMsgToBeSent;
	int anaCallReturn;
	unsigned char *unprocessedMessageBuffer = message;
	unsigned char *messageToBeSent[8];
	//canMsgToBeSent.m_dwID = cobID;
	//canMsgToBeSent.m_bDLC = len;
	int flags = 0x0;
	if (rtr)
	{
		flags = 2; // � Bit 1: If set, the telegram is marked as remote frame.
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
		//anaCallReturn = CANWrite(m_UcanHandle, cobID, reinterpret_cast<char*>(messageToBeSent), messageLengthToBeProcessed, flags);
		anaCallReturn = 0;
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

bool mockUp::sendRemoteRequest(short cobID)
{
	int anaCallReturn;
	int flags = 2;// Bit 1: If set, the telegram is marked as remote frame.

	//Status = UcanWriteCanMsgEx(m_UcanHandle, m_channelNumber, &canMsg, NULL);
	//anaCallReturn = CANWrite(m_UcanHandle,cobID,NULL, 0, flags);
	anaCallReturn = 0;
	if (anaCallReturn != 0)
	{
		MLOG(ERR,this) << "Error: There was a problem when sending a message.";
	}
	return sendErrorCode(anaCallReturn);
}

bool mockUp::errorCodeToString(long error, char message[])//TODO: Fix this method, this doesn't do anything!!
{
  char tmp[300];
// canGetErrorText((canStatus)error, tmp, sizeof(tmp));
// *message = new char[strlen(tmp)+1];	
    message[0] = 0;
// strcpy(*message,tmp);
  return true;
}

void mockUp::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_baudRate);
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}

bool mockUp::initialiseLogging(LogItInstance* remoteInstance)
{
	bool ret = Log::initializeDllLogging(remoteInstance);
	LOG(Log::INF) << "DLL: logging initialised";
	return ret;
}
