#include "MockCanAccess.h"

//////////////////////////////////////////////////////////////////////
// MockCanAccess.cpp: implementation of the MockCanAccess class.
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
#endif

extern "C" DLLEXPORTFLAG CanModule::CCanAccess *getCanbusAccess()
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

bool MockCanAccess::createBUS(const char *name,const char *parameters)
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
	LOG(Log::DBG) << __FUNCTION__ << " cobID ["<<cobID<<"] len ["<<(unsigned int)len<<"] message [0x"<<std::hex<<message<<std::dec<<"] rtr ["<<rtr<<"]";
	m_statistics.onTransmit(len);
	return true;
}

void MockCanAccess::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_baudRate);
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}

bool MockCanAccess::initialiseLogging(LogItInstance* remoteInstance)
{
	return true;
}
