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
	virtual bool createBus(const string name, const string parameters);
	virtual bool sendRemoteRequest(short cobID);
    virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);
	virtual void getStatistics( CanModule::CanStatistics & result );
	virtual bool initialiseLogging(LogItInstance* remoteInstance);

	static std::string getCanMessageDataAsString(const unsigned char* data, const unsigned char& len = 8);

private:
	//Instance of Can Statistics
	CanModule::CanStatistics m_statistics;

	//Current baud rate
	unsigned int m_baudRate;
};

#endif //MOCKCANACCESS_H_
