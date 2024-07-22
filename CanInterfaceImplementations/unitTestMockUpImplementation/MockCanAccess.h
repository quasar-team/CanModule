/*
 * MockCanAccess.h
 *
 *  Created on: Feb 23, 2018
 *      Author: ben farnham
 */

#ifndef MOCKCANACCESS_H_
#define MOCKCANACCESS_H_

#include <string>
#include "CanStatistics.h"
#include "CCanAccess.h"

#ifdef _WIN32

#include "tchar.h"
#include "Winsock2.h"
#include "windows.h"

#else

typedef unsigned long DWORD;

#endif

#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;

#define CAN_ECHO_MSG (unsigned char*)("0000EC40")

/*
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus access layer that will communicate with Systec crates (Windows only)
 */
class MockCanAccess: public CanModule::CCanAccess
{
public:
	MockCanAccess();
	virtual ~MockCanAccess();

	//Disable copy constructor && assignment
	MockCanAccess(MockCanAccess const & other) = delete;
	MockCanAccess& operator=(MockCanAccess const & other) = delete;

	/*
	 * Required overrides of abstract base class CCanAccess
	 */
	virtual int createBus(const std::string name, const std::string parameters);
	virtual int createBus(const std::string name, const std::string parameters, float factor );
	virtual bool sendRemoteRequest(uint32_t cobID);
    virtual bool sendMessage(uint32_t cobID, unsigned char len, unsigned char *message, bool rtr = false, bool eff = false);
	virtual void getStatistics( CanModule::CanStatistics & result );
	virtual bool initialiseLogging(LogItInstance* remoteInstance);

	static std::string getCanMessageDataAsString(const unsigned char* data, const unsigned char& len = 8);
	virtual uint32_t getPortStatus(){ return 0xbebecaca; };
	virtual uint32_t getPortBitrate(){ return m_CanParameters.m_lBaudRate; };


	virtual void stopBus ();
	virtual void fetchAndPublishCanPortState (){};
	virtual std::vector<CanModule::PORT_LOG_ITEM_t> getHwLogMessages ( unsigned int n );
	virtual CanModule::HARDWARE_DIAG_t getHwDiagnostics ();
	virtual CanModule::PORT_COUNTERS_t getHwCounters ();


private:
	//Instance of Can Statistics
	CanModule::CanStatistics m_statistics;

	//Current baud rate
	unsigned int m_baudRate;
	boost::posix_time::ptime m_now, m_previous;

	static int st_CCanAccess_icount;
};

#endif //MOCKCANACCESS_H_
