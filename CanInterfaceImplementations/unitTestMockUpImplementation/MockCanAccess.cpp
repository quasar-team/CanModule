#include "MockCanAccess.h"

//////////////////////////////////////////////////////////////////////
// MockCanAccess.cpp: implementation of the MockCanAccess class.
//////////////////////////////////////////////////////////////////////

#include <time.h>
#include <cstring>
#include <string.h>

#include <map>
#include <LogIt.h>
#include <sstream>
#include <iostream>

#ifdef _WIN32
  #define DLLEXPORTFLAG __declspec(dllexport)
#else
  #define DLLEXPORTFLAG
#endif

using namespace CanModule;

extern "C" DLLEXPORTFLAG CanModule::CCanAccess *getCanBusAccess()
{
	static MockCanAccess g_sMockCanAccess;
	return &g_sMockCanAccess;
}

MockCanAccess::MockCanAccess()
:CanModule::CCanAccess(),m_baudRate(0)
{
	m_statistics.setTimeSinceOpened();
	m_statistics.beginNewRun();
	m_failedSendCountdown = m_maxFailedSendCount;
}

void MockCanAccess::stopBus (){
	LOG(Log::TRC) << __FUNCTION__ << " implementation specific (doing nothing in this case)";
}

std::vector<CanModule::PORT_LOG_ITEM_t> MockCanAccess::getHwLogMessages ( unsigned int n ){
	std::vector<CanModule::PORT_LOG_ITEM_t> log;
	return log;
}
CanModule::HARDWARE_DIAG_t MockCanAccess::getHwDiagnostics (){
	HARDWARE_DIAG_t d;
	return d;
}


MockCanAccess::~MockCanAccess()
{
	LOG(Log::DBG) << __FUNCTION__ << " dtor called, calling stopBus()";
	stopBus ();
}

int MockCanAccess::createBus(const std::string name, const std::string parameters)
{	
	LOG(Log::INF) << __FUNCTION__ << " name [" << name << "] parameters [" << parameters << "]";
	m_sBusName = name;
	return 0;
}


bool MockCanAccess::sendRemoteRequest(short cobID)
{
	LOG(Log::DBG) << __FUNCTION__ << " Sending cobID: ["<<cobID <<"]";
	m_statistics.onTransmit(sizeof(cobID));
	return true;
}

bool MockCanAccess::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	LOG(Log::DBG) << __FUNCTION__ << " Sending message: [" << ( message == 0  ? "" : (const char *) message) << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";
	m_statistics.onTransmit(len);

	if(0 == memcmp(message, CAN_ECHO_MSG, len))
	{
		CanMessage response;
		response.c_id = cobID;
		response.c_dlc = len;
		memcpy(response.c_data, message, (unsigned int)len);
		canMessageCame(response);
	}
	return true;
}

void MockCanAccess::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_baudRate);
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}

std::string MockCanAccess::getCanMessageDataAsString(const unsigned char* data, const unsigned char& len/*8*/)
{
	return std::string(data, data + (len*sizeof(unsigned char)));
}

bool MockCanAccess::initialiseLogging(LogItInstance* remoteInstance)
{
	return true;
}
