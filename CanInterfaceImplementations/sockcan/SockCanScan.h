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

#include "CCanAccess.h"
#include "CanStatistics.h"
#include "libsocketcan.h"

/*
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus access layer that will communicate with socket can (Linux only)
 */
using namespace CanModule;

class CSockCanScan : public CCanAccess
{
public:

	CSockCanScan(); //Constructor of the class. Will initiate the statistics.
	CSockCanScan(CSockCanScan const & other) = delete; 	//Disables copy constructor
	CSockCanScan& operator=( CSockCanScan const & other) = delete; //Disables assignment
	virtual ~CSockCanScan();

	virtual bool sendRemoteRequest(short cobID);
	virtual int createBus(const string name ,string parameters );
	virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);
	virtual void getStatistics( CanStatistics & result );

	/**
	 * return socketcan port status in vendor unified format (with implementation nibble set), from can_netlink.h
	 * do not start a new statistics run for that.
	 * enum can_state {
	 * CAN_STATE_ERROR_ACTIVE = 0,	 RX/TX error count < 96
	 * CAN_STATE_ERROR_WARNING,	 RX/TX error count < 128
	 * CAN_STATE_ERROR_PASSIVE,	 RX/TX error count < 256
	 * CAN_STATE_BUS_OFF,		 RX/TX error count >= 256
	 * CAN_STATE_STOPPED,		 Device is stopped
	 * CAN_STATE_SLEEPING,		 Device is sleeping
	 * CAN_STATE_MAX
	 * };
		 *
	 * update the CAN bus status IFLA from socketcan and make up a CanModule
	 * status bitpattern out from this.
	 *
	 * int can_get_restart_ms(const char *name, __u32 *restart_ms);
	 * int can_get_bittiming(const char *name, struct can_bittiming *bt);
	 * int can_get_ctrlmode(const char *name, struct can_ctrlmode *cm);
	 * int can_get_state(const char *name, int *state);
	 * int can_get_clock(const char *name, struct can_clock *clock);
	 * int can_get_bittiming_const(const char *name, struct can_bittiming_const *btc);
	 * int can_get_berr_counter(const char *name, struct can_berr_counter *bc);
	 * int can_get_device_stats(const char *name, struct can_device_stats *cds);
	 *
	 */
	virtual uint32_t getPortStatus(){
		int _state;
		can_get_state( m_channelName.c_str(), &_state );
		return( _state | CANMODULE_STATUS_BP_SOCK );
	};

	virtual uint32_t getPortBitrate(){ return m_CanParameters.m_lBaudRate; };

	/**
	 * Returns socket handler
	 */
	int getHandler() { return m_sock; }

	/**
	 * produce an empty can frame
	 */
	static can_frame emptyCanFrame( void ){
		can_frame f;
		f.can_dlc = 0;
		f.can_id = 0;
		for ( int i = 0; i < 8; i++){
			f.data[ i ] = 0;
		}
		return(f);
	}
	static std::map<string, string> m_busMap; // {name, parameters}
	Log::LogComponentHandle logItHandle() { return m_logItHandleSock; }

	virtual void setReconnectBehavior( CanModule::ReconnectAutoCondition cond, CanModule::ReconnectAction action ){
		m_reconnectCondition = cond;
		m_reconnectAction = action;
	};
	virtual void setReconnectReceptionTimeout( unsigned int timeout ){ 	m_timeoutOnReception = timeout;	};
	virtual void setReconnectFailedSendCount( unsigned int c ){
		m_maxFailedSendCount = m_failedSendCountdown = c;
		std::cout << __FILE__ << " " << __LINE__ << " m_triggerCounter= " << m_failedSendCountdown << std::endl;
	}
	virtual CanModule::ReconnectAutoCondition getReconnectCondition() { return m_reconnectCondition; };
	virtual CanModule::ReconnectAction getReconnectAction() { return m_reconnectAction; };

	virtual void stopBus ();

private:
	volatile atomic_bool m_CanScanThreadRunEnableFlag; //Flag for running/shutting down the

	// socket: CanScan thread, with compiler mem optimization switched off (~atomic) for more code safety
	atomic_int m_sock;
	int m_errorCode; // As up-to-date as possible state of the interface.

	CanStatistics m_statistics;
	std::thread *m_hCanScanThread;
	std::string m_channelName;
	std::string m_busName;
	Log::LogComponentHandle m_logItHandleSock;

	void updateInitialError () ;
	static std::string errorFrameToString (const struct can_frame &f);

	void sendErrorMessage(const char  *);
	void clearErrorMessage();
	int configureCanBoard(const string name,const string parameters);
	// void updateBusStatus();
	int openCanPort();
	void CanScanControlThread(); // not static, private is enough in C11
	void CanReconnectionThread();// not static, private is enough in C11

};


#endif /* SOCKCANSCAN_H_ */
