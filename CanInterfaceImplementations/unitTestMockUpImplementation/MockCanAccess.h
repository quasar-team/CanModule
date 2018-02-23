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
	virtual bool createBUS(const char * name ,const char *parameters);
	virtual bool sendRemoteRequest(short cobID);
    virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);
	virtual void getStatistics( CanStatistics & result );
	virtual bool initialiseLogging(LogItInstance* remoteInstance);

private:
	//Instance of Can Statistics
	CanStatistics m_statistics;

	//Current baud rate
	unsigned int m_baudRate;
};

#endif //MOCKCANACCESS_H_
