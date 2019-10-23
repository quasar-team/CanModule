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

#include <pthread.h>
#include <unistd.h>
#include <string>
#include <sys/socket.h>
#include <linux/can.h>
#
#include <boost/thread/thread.hpp>


#include "CCanAccess.h"
#include "CanStatistics.h"
include "libsocketcan.h"

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
	virtual bool createBus(const string name ,string parameters );
	virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);

	/**
	 * Returns socket handler
	 */
	int getHandler() { return m_sock; }

	/**
	 * Returns the instance of the CanStatistics object
	 */
	virtual void getStatistics( CanStatistics & result );

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

private:
	volatile bool m_CanScanThreadRunEnableFlag; //Flag for running/shutting down the CanScan thread

	int m_sock;                 //Socket handler
	CanStatistics m_statistics;// Instance of Can Statistics
	pthread_t m_hCanScanThread;	// Handle for the CAN update scan manager thread.
	int m_idCanScanThread; // Thread ID for the CAN update scan manager thread.
	int m_errorCode; // As up-to-date as possible state of the interface.
	std::string m_channelName;
	std::string m_busName;

	static Log::LogComponentHandle st_logItHandleSock;

	//Closeup method that will be called from the destructor.
	bool stopBus ();
	//Report an error when opening a can port
	void updateInitialError () ;
	//Transforms an error frame into an error message (string format)
	static std::string errorFrameToString (const struct can_frame &f);

	void sendErrorMessage(const char  *);
	//   void sendErrorMessage(const struct can_frame *);

	void clearErrorMessage();

	int configureCanBoard(const string name,const string parameters);

	/** Obtains a SocketCAN socket and opens it.
	 *  The name of the port and parameters should have been specified by preceding call to configureCanboard()
	 *
	 *  @returns less than zero in case of error, otherwise success
	 */
	int openCanPort();

	//The main control thread function for the CAN update scan manager.
	static void* CanScanControlThread(void *);


};


#endif /* SOCKCANSCAN_H_ */
