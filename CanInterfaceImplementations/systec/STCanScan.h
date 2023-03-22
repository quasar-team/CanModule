/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * STCanScap.h
 *
 *  Created on: Feb 22, 2012
 *      Author: vfilimon
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
#ifndef CCANSTSCAN_H_
#define CCANSTSCAN_H_

#include "tchar.h"
#include "Winsock2.h"
#include "windows.h"
#include <string>
#include "CanStatistics.h"
// #include <Usbcan32.h>
#include <Usbcan64.h>
#include "CCanAccess.h"

using namespace CanModule;

/*
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus access layer that will communicate with Systec crates (Windows only)
 */
class STCanScan: public CanModule::CCanAccess
{
public:
	STCanScan();
	STCanScan(STCanScan const & other) = delete; //Disables copy constructor
	STCanScan& operator=(STCanScan const & other) = delete; //Disables assignment
	virtual ~STCanScan();

	virtual int createBus(const std::string name ,const std::string parameters);
    virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);

    /*
	 * Method that sends a remote request trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
	 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
	 * @param cobID: Identifier that will be used for the request.
	 * @return: Was the initialisation process successful?
	 */
	virtual bool sendRemoteRequest(short cobID);
	//Returns the instance of the CanStatistics object
	virtual void getStatistics( CanStatistics & result );
	// unified status
	virtual uint32_t getPortStatus();
	virtual uint32_t getPortBitrate(){ return m_CanParameters.m_lBaudRate; };

	Log::LogComponentHandle logItHandle(){ return m_logItHandleSt; }
	virtual void setReconnectBehavior( CanModule::ReconnectAutoCondition cond, CanModule::ReconnectAction action ){
		m_reconnectCondition = cond;
		m_reconnectAction = action;
	};
	virtual void setReconnectReceptionTimeout( unsigned int timeout ){ 	m_timeoutOnReception = timeout;	};
	virtual void setReconnectFailedSendCount( unsigned int c ){ m_maxFailedSendCount = m_failedSendCountdown = c;	}
	virtual CanModule::ReconnectAutoCondition getReconnectCondition() { return m_reconnectCondition; };
	virtual CanModule::ReconnectAction getReconnectAction() { return m_reconnectAction; };
	virtual void stopBus();
	virtual void fetchAndPublishCanPortState ();

private:

	int m_moduleNumber;
	int m_channelNumber;
	int m_canHandleNumber;
	tUcanHandle m_UcanHandle;
	CanStatistics m_statistics;
	unsigned int m_baudRate;
	Log::LogComponentHandle m_logItHandleSt;
	bool sendErrorCode(long);

	DWORD	m_busStatus;
	bool m_CanScanThreadShutdownFlag;
    // Handle for the CAN update scan manager thread.
    HANDLE	m_hCanScanThread;
    // Thread ID for the CAN update scan manager thread.
    DWORD   m_idCanScanThread;
    // The main control thread function for the CAN update scan manager.
	static DWORD WINAPI CanScanControlThread(LPVOID pCanScan);
	static tUcanInitCanParam createInitializationParameters( unsigned int br );

	int configureCanBoard(const std::string name,const std::string parameters);
	int openCanPort(tUcanInitCanParam initializationParameters);

	/*
	 * Provides textual representation of an error code.
	 */
	const char * errorCodeToString(long error) {return( STcanGetErrorText( error ).c_str() ); }

	static void setCanHandleInUse(int n,bool t) { s_isCanHandleInUseArray[n] = t; }
	static bool isCanHandleInUse(int n) { return s_isCanHandleInUseArray[n]; }
	static void setCanHandle(int n,tUcanHandle tU) { s_canHandleArray[n] = tU; }
	static tUcanHandle getCanHandle(int n) { return s_canHandleArray[n]; }

	static tUcanHandle s_canHandleArray[256];
	static bool s_isCanHandleInUseArray[256];

	static int reconnectAllPorts( tUcanHandle h );
	unsigned int vendorBaudRate2UserBaudRate( unsigned int vb );
	std::string STcanGetErrorText( long errCode );


};

#endif
