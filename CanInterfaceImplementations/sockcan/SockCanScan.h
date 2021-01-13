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

#if 0
#include <pthread.h>
#endif

#include <thread>
#include <string>

#include <unistd.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <boost/thread/thread.hpp> // for cross platform sleep


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
	 * return socketcan port status as-is, from can_netlink.h
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
	 */
	virtual uint32_t getPortStatus(){
		CanStatistics stats;
		getStatistics( stats );
		return( stats.portStatus() | CANMODULE_STATUS_BP_SOCK );
	};

	virtual uint32_t getPortBitrate(){ return m_CanParameters.m_lBaudRate; };

	/**
	 * Returns socket handler
	 */
	int getHandler() { return m_sock; }

	/**
	 * produce and empty can frame
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
		m_failedSendCounter = m_triggerCounter = c;
		std::cout << __FILE__ << " " << __LINE__ << " m_triggerCounter= " << m_triggerCounter << std::endl;
	}
	virtual CanModule::ReconnectAutoCondition getReconnectCondition() { return m_reconnectCondition; };
	virtual CanModule::ReconnectAction getReconnectAction() { return m_reconnectAction; };

private:
	volatile bool m_CanScanThreadRunEnableFlag; //Flag for running/shutting down the CanScan thread

	int m_sock;                 //Socket handler
	CanStatistics m_statistics;// Instance of Can Statistics
	std::thread *m_hCanScanThread;	// ptr thread object, not a thread
	int m_errorCode; // As up-to-date as possible state of the interface.
	std::string m_channelName;
	std::string m_busName;

	Log::LogComponentHandle m_logItHandleSock;


	/**
	 * Closeup method that will be called from the destructor.
	 */
	bool stopBus ();

	/**
	 * Report an error when opening a can port
	 */
	void updateInitialError () ;

	/**
	 * Transforms an error frame into an error message (string format)
	 */
	static std::string errorFrameToString (const struct can_frame &f);

	void sendErrorMessage(const char  *);
	void clearErrorMessage();
	int configureCanBoard(const string name,const string parameters);
	void updateBusStatus();

	/** Obtains a SocketCAN socket and opens it.
	 *  The name of the port and parameters should have been specified by preceding call to configureCanboard()
	 *
	 *  @returns less than zero in case of error, otherwise success
	 */
	int openCanPort();

	/**
	 * The main control thread function for the CAN update scan manager.
	 */
	static void CanScanControlThread(void *);


};


#endif /* SOCKCANSCAN_H_ */
