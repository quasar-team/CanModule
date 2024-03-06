/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * SockCanScan.h
 *
 *  Created on: Jul 21, 2011
 *  Based on work by vfilimon
 *  Rework and logging done by Piotr Nikiel <piotr@nikiel.info>
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
#ifndef SOCKCANSCAN_H_
#define SOCKCANSCAN_H_

#include <thread>
#include <mutex>
#include <condition_variable>

#include <string>

#include <unistd.h>
#include <sys/socket.h>
#include <linux/can.h>

#include <CCanAccess.h>
#include <CanStatistics.h>
#include <libsocketcan.h>
#include <Diag.h>

#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;

/*
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus access layer that will communicate with socket can (Linux only)
 */
using namespace CanModule;

class CSockCanScan : public CCanAccess
{
public:
	static std::map<std::string, std::string> m_busMap; // {name, parameters}
	static can_frame emptyCanFrame( void );

	CSockCanScan(); //Constructor of the class. Will initiate the statistics.
	CSockCanScan(CSockCanScan const & other) = delete; 	//Disables copy constructor
	CSockCanScan& operator=( CSockCanScan const & other) = delete; //Disables assignment
	virtual ~CSockCanScan();

	virtual bool sendRemoteRequest(short cobID);
	virtual int createBus(const std::string name, const std::string parameters );
	virtual int createBus(const std::string name, const std::string parameters, float factor );
	virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);
	virtual void getStatistics( CanStatistics & result );
	virtual uint32_t getPortStatus();
	virtual uint32_t getPortBitrate(){ return m_CanParameters.m_lBaudRate; };
#if 0
	virtual void setReconnectBehavior( CanModule::ReconnectAutoCondition cond, CanModule::ReconnectAction action );
	virtual void setReconnectReceptionTimeout( unsigned int timeout ){ 	m_timeoutOnReception = timeout;	};
	virtual void setReconnectFailedSendCount( unsigned int c ){	m_maxFailedSendCount = m_failedSendCountdown = c;	}
	virtual CanModule::ReconnectAutoCondition getReconnectCondition() { return m_reconnectCondition; };
	virtual CanModule::ReconnectAction getReconnectAction() { return m_reconnectAction; };
#endif

	virtual void stopBus ();
	virtual void fetchAndPublishCanPortState ();

	virtual std::vector<CanModule::PORT_LOG_ITEM_t> getHwLogMessages ( unsigned int n );
	virtual CanModule::HARDWARE_DIAG_t getHwDiagnostics ();
	virtual CanModule::PORT_COUNTERS_t getHwCounters ();

	int getHandler() { return m_sock; }
	Log::LogComponentHandle logItHandle() { return m_logItHandleSock; }
	void updateInitialError () ; // public again

private:
	volatile std::atomic_bool m_CanScanThreadRunEnableFlag; //Flag for running/shutting down the

	// socket: CanScan thread, with compiler mem optimization switched off (~atomic) for more code safety
	std::atomic_int m_sock;

	int m_canMessageErrorCode;
	CanStatistics m_statistics;
	std::thread *m_hCanScanThread;
	std::string m_channelName;
	std::string m_busName;
	Log::LogComponentHandle m_logItHandleSock;
	GlobalErrorSignaler *m_gsig;
	boost::posix_time::ptime m_now, m_previous;

	static std::string m_canMessageErrorFrameToString (const struct can_frame &f);

	void m_sendErrorMessage(const char  *);
	void m_clearErrorMessage();
	int m_configureCanBoard(const std::string name, const std::string parameters);
	int m_openCanPort();
	void m_CanScanControlThread(); // not static, private is enough in C11
	void m_CanReconnectionThread();// not static, private is enough in C11
	bool m_writeWrapper (const can_frame* frame);
	int m_selectWrapper ();
};


#endif /* SOCKCANSCAN_H_ */
