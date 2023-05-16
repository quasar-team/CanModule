/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * AnaCanScan.h
 *
 *  Created on: Feb 22, 2012
 *      Author: vfilimon, quasar team
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

#ifndef CCANANASCAN_H_
#define CCANANASCAN_H_


#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <time.h>
#include <map>
#include <LogIt.h>
#include <sstream>
#include <iostream>


#include <CanModuleUtils.h>
#include <CCanAccess.h>

#include "CanStatistics.h"
#include "CCanAccess.h"
#include "CanMessage.h"
#include <Diag.h>

// specific vendor

#ifdef _WIN32
// #include <AnaGate.h>
#include <AnaGateDLL.h>
#include <AnaGateDllCan.h>
#else
#include "AnaGateDLL.h"
#include "AnaGateDllCan.h"
#endif

#include "AnaGateErrors.h"

#ifdef _WIN32

#include "tchar.h"
#include "Winsock2.h"
#include "windows.h"

#else


typedef unsigned long DWORD;
#endif

/**
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus
 * access layer that will communicate with AnaGate bridges over TCP/IP, Linux and Windows.
 */
using namespace CanModule;

/**
 * one CAN port on an anagate bridge, an anagate bridge is an ip number. There
 * are (physical) bridges with several ip numbers: we treat them as separated modules.
 */
class AnaCanScan: public CanModule::CCanAccess
{

public:

	// Returns the current network connection state. The following values are possible:
	enum ANAGATE_API_V2_DEVICE_CONNECTION_STATE {
		DISCONNECTED = 1, //  The connection to the AnaGate is disconnected.
		CONNECTING, // : The connection is connecting.
		CONNECTED, // : The connection is established.
		DISCONNECTING, //: The connection is disonnecting.
		NOT_INITIALIZED }; //: The network protocol is not successfully initialized.

	AnaCanScan();//Constructor of the class. Will initiate the statistics.
	AnaCanScan(AnaCanScan const & other) = delete;  //Disables copy constructor
	AnaCanScan& operator=(AnaCanScan const & other) = delete; // Disables assignment
	virtual ~AnaCanScan();

	virtual int createBus(const std::string name, const std::string parameters);
    virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);
	virtual bool sendMessage(CanMessage *);
	virtual bool sendRemoteRequest(short cobID);
	virtual void getStatistics( CanStatistics & result );

	void statisticsOnRecieve(int);
	void callbackOnRecieve(CanMessage&);

	uint32_t getPortStatus();
	virtual uint32_t getPortBitrate(){ return m_CanParameters.m_lBaudRate; };

	virtual void setReconnectBehavior( CanModule::ReconnectAutoCondition cond, CanModule::ReconnectAction action ){
		m_reconnectCondition = cond;
		m_reconnectAction = action;
	};
	virtual void setReconnectReceptionTimeout( unsigned int timeout ){ 	m_timeoutOnReception = timeout;	};
	virtual void setReconnectFailedSendCount( unsigned int c ){ m_maxFailedSendCount = m_failedSendCountdown = c;	}
	virtual CanModule::ReconnectAutoCondition getReconnectCondition() { return m_reconnectCondition; };
	virtual CanModule::ReconnectAction getReconnectAction() { return m_reconnectAction; };
	virtual void stopBus( void );
	virtual void fetchAndPublishCanPortState ();

	virtual std::vector<PORT_LOG_ITEM_t> getHwLogMessages ( int n );

private:

 	int m_canPortNumber;       //The number of can port (CANA, CANB, ...) associated with this instance.
 	std::string  m_canIPAddress;
	unsigned int m_baudRate;   //Current baud rate for statistics
	DWORD   m_idCanScanThread; // Thread ID for the CAN update scan manager thread.
	bool m_canCloseDevice;
	std::string m_busName;
	std::string m_busParameters;
	AnaInt32 m_UcanHandle;     // handler of the can bus
	CanStatistics m_statistics; //Instance of Can Statistics
    AnaInt32 m_timeout; 		// connect_wait time
    bool m_busStopped;
	GlobalErrorSignaler *m_gsig;


    /**
     * we would like to keep logging from a few static methods as well, since one IP
     * number corresponds to many ports and some class-level management is going on for
     * the connect-reconnect behavior. We definitely need these to be static. So since we
     * need the logit handle static there is no point in having it also private. At least the
     * cross-thread references to the handler are within this class only. It is
     * all re-entrant code, and the handler does not change during runtime any more, so no
     * mutex protection for now.
     */
    static Log::LogComponentHandle st_logItHandleAnagate;


	/**
	 * can handle map: handles are unique on one host and depend on {port, ip}. We can use the handle as a key
	 * therefore.
	 * we have one object for each CAN port, each object has it's handle, which are unique system-wide.
	 * Several objects thus share the same ip. CAN { port, ip} and object handles are therefore equivalent,
	 * except that the handle value is assigned by the OS at CANopen.
	 */
	typedef struct {
		AnaInt32 port;
		std::string ip;
	} ANAGATE_PORTDEF_t;
	static std::map<int, ANAGATE_PORTDEF_t> st_canHandleMap;
	static std::map<std::string, bool> st_reconnectInProgress_map; // could use 1-dim vector but map is faster
	static void st_showAnaCanScanObjectMap();
	static int st_reconnectAllPorts( std::string ip );
	static void st_setIpReconnectInProgress( std::string ip, bool flag );
	static bool st_isIpReconnectInProgress( std::string ip );
	static void st_addCanHandleOfPortIp(AnaInt32 handle, int port, std::string ip);
	static void st_deleteCanHandleOfPortIp(int port, std::string ip);
	static AnaInt32 st_getCanHandleOfPortIp(int port, std::string ip);

	AnaInt32 m_reconnectThisPort();
	void m_eraseReceptionHandlerFromMap( AnaInt32 h );
	void m_CanReconnectionThread();               // not static, private is enough in C11
	void m_signalErrorMessage( int code, std::string msg );
	std::string m_ipAddress(){ return(m_canIPAddress );}
	int m_CanPortNumber(){ return(m_canPortNumber);}
	int m_handle(){ return(m_UcanHandle);}
	int m_configureCanBoard(const std::string name,const std::string parameters);
	int m_connectReceptionHandler();
	int m_openCanPort();
	int m_reconnect();

};

#endif //CCANANASCAN_H_
