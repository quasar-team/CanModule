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
	m_statistics.beginNewRun();
}

MockCanAccess::~MockCanAccess()
{
	LOG(Log::DBG) << "Destroying MockCanAccess object";
}

bool MockCanAccess::createBus(const string name, const string parameters)
{	
	LOG(Log::INF) << __FUNCTION__ << " called with name [" << name << "] parameters [" << parameters << "]";
	return true;
}

bool MockCanAccess::sendRemoteRequest(short cobID)
{
	LOG(Log::DBG) << __FUNCTION__ << " Sending cobID: ["<<cobID <<"]";
	m_statistics.onTransmit(sizeof(cobID));
	return true;
}

bool MockCanAccess::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	LOG(Log::DBG) << __FUNCTION__ << " cobID ["<<cobID<<"] len ["<<(unsigned int)len<<"] message [0x"<<getCanMessageDataAsString(message, len)<<"] rtr ["<<rtr<<"]";
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
