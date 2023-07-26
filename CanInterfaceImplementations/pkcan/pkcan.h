/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 * pkcan.h
 *
 *  Created on: JUL 2, 2012
 *      Author: vfilimon, mludwig
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

#ifndef CCANPKSCAN_H_
#define CCANPKSCAN_H_


#include <string>
#include "Winsock2.h"
#include "windows.h"

#include <PCANBasic.h>
#include "CCanAccess.h"
#include "CanStatistics.h"
#include <Diag.h>

#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;

using namespace CanModule;

/*
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus access layer that will communicate with Peak USB-CAN devices (Windows only)
 */
class PKCanScan: public CanModule::CCanAccess
{
public:
	PKCanScan();
	PKCanScan(PKCanScan const & other) = delete; //Disables copy constructor
	PKCanScan& operator=(PKCanScan const & other) = delete;//Disables assignment
	virtual ~PKCanScan();

	virtual int createBus(const std::string name,const std::string parameters);
	virtual int createBus(const std::string name, const std::string parameters, bool lossless );
	virtual int createBus(const std::string name, const std::string parameters, float factor );

	/*
	 * Method that sends a message trough the can bus channel. If the method createBus was not called before this, sendMessage will fail, as there is no
	 * can bus channel to send a message trough.
	 * @param cobID: Identifier that will be used for the message.
	 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
	 * @param message: Message to be sent trough the can bus.
	 * @param rtr: is the message a remote transmission request?
	 * @return: Was the sending process successful?
	 */
    virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);
    /*
	 * Method that sends a remote request trough the can bus channel. If the method createBus was not called before this, sendMessage will fail, as there is no
	 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
	 * @param cobID: Identifier that will be used for the request.
	 * @return: Was the initialisation process successful?
	 */
	virtual bool sendRemoteRequest(short cobID);
	//Returns the instance of the CanStatistics object
	virtual void getStatistics( CanStatistics & result );
	virtual uint32_t getPortStatus();
	virtual uint32_t getPortBitrate(){ return m_CanParameters.m_lBaudRate; };

	/*
	 * Converts Error code into a text message.
	 */
	bool getErrorMessage(long error, char **message);

	static std::map<std::string, std::string> m_busMap; // {name, parameters}
	Log::LogComponentHandle logItHandle(){ return m_logItHandlePk; }
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

	virtual std::vector<CanModule::PORT_LOG_ITEM_t> getHwLogMessages ( unsigned int n );
	virtual CanModule::HARDWARE_DIAG_t getHwDiagnostics ();
	virtual CanModule::PORT_COUNTERS_t getHwCounters ();

	int getFailedSendCountdown() { return m_failedSendCountdown; }
	TPCANHandle	getTPCANHandle() { return m_pkCanHandle;	}
	unsigned int getBaudRate() { return m_baudRate; }

private:

	Log::LogComponentHandle m_logItHandlePk;

	// The main control thread function for the CAN update scan manager.
	static DWORD WINAPI CanScanControlThread(LPVOID pCanScan);
	// reconnection thread
	static DWORD WINAPI CanReconnectionThread(LPVOID pCanScan);

	CanStatistics m_statistics;
	TPCANStatus m_busStatus;
	bool m_CanScanThreadRunEnableFlag;

	unsigned int m_baudRate;
	std::string m_busName;
	std::string m_busParameters;
   	TPCANHandle	m_pkCanHandle;

	HANDLE      m_hCanScanThread;
	HANDLE m_PeakReconnectionThread;
    DWORD           m_idCanScanThread;
    DWORD           m_idPeakReconnectionThread;
	GlobalErrorSignaler *m_gsig;
	boost::posix_time::ptime m_now, m_previous;

	TPCANHandle m_getHandle(const char *name);
	bool m_sendErrorCode(long);
	bool m_configureCanboard(const std::string name, const std::string parameters);
    std::string m_translatePeakPortStatusBitpatternToText( TPCANStatus bp );
};

#endif
