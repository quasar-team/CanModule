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
#include "CanStatistics.h"
#include "CCanAccess.h"
#include "CanMessage.h"

#ifdef _WIN32

// #include "AnaGateDllCAN.h"
#include "AnaGateDllCan.h"
#include "AnaGateDll.h"
#include "tchar.h"
#include "Winsock2.h"
#include "windows.h"

#else

#include "AnaGateDLL.h"
#include "AnaGateDllCan.h"

typedef unsigned long DWORD;
#endif

/**
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus
 * access layer that will communicate with AnaGate bridges over TCP/IP (Windows only)
 */
using namespace CanModule;


class AnaCanScan: public CanModule::CCanAccess
{

public:
	AnaCanScan();//Constructor of the class. Will initiate the statistics.
	AnaCanScan(AnaCanScan const & other) = delete;  //Disables copy constructor
	AnaCanScan& operator=(AnaCanScan const & other) = delete; // Disables assignment
	virtual ~AnaCanScan();

	virtual bool createBus(const string name ,const string parameters);
    virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);
	virtual bool sendMessage(CanMessage *);
	virtual bool sendRemoteRequest(short cobID);
	virtual void getStatistics( CanStatistics & result );

	void statisticsOnRecieve(int);
	void callbackOnRecieve(CanMessage&);
	void startAlive( int aliveTime_sec );
	void setConnectWaitTime( int timeout_ms );

private:
	int m_canPortNumber; //The number of can port (CANA, CANB, ...) associated with this instance.
	string  m_canIPAddress;
	unsigned int m_baudRate; 	//Current baud rate for statistics
	DWORD   m_idCanScanThread; // Thread ID for the CAN update scan manager thread.
	bool m_canCloseDevice;
	string m_busName;
	string m_busParameters;
	AnaInt32 m_UcanHandle; //Instance of the can handle
	CanStatistics m_statistics; //Instance of Can Statistics
    AnaInt32 m_timeout; 		// connect_wait time
	static Log::LogComponentHandle s_logItHandleAnagate;
	static bool s_logItRegisteredAnagate;
	static AnaInt32 s_canHandleArray[256];
	static bool s_isCanHandleInUseArray[256];

	static void objectMapSize();
	static int reconnectAllPorts( string ip );

	/**
	 * we have one object for each CAN port, each object has an unique handle. Several objects thus share
	 * the same ip. CAN ports and object handles are therefore equivalent, except that the handle value is
	 * assigned by the OS at CANopen.
	 */
	static void setCanHandleInUse(int handle, bool flag) { s_isCanHandleInUseArray[ handle ] = flag; }
	static bool isCanHandleInUse(int handle) { return s_isCanHandleInUseArray[ handle ]; }
	static void setCanHandleOfPort(int port, AnaInt32 handle) { s_canHandleArray[ port ] = handle; }
	static AnaInt32 getCanHandleFromPort(int n) { return s_canHandleArray[n]; }


	static std::map<string, bool> reconnectInProgress_map; // could use 1-dim vector but map is faster
	static void setIpReconnectInProgress( string ip, bool flag );
	static bool isIpReconnectInProgress( string ip );
	static std::string canMessageToString(CanMessage &f);

	bool sendErrorCode(AnaInt32);
	string ipAdress(){ return(m_canIPAddress );}
	int canPortNumber(){ return(m_canPortNumber);}
	int handle(){ return(m_UcanHandle);}
	int configureCanBoard(const string name,const string parameters);
	int connectReceptionHandler();
	int openCanPort();
	int reconnect();
	bool errorCodeToString(long error, char message[]);
	void stopBus( void );
	void eraseReceptionHandlerFromMap( AnaInt32 h );

};

#endif //CCANANASCAN_H_
