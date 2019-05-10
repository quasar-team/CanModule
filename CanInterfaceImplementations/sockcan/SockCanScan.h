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
#include "CCanAccess.h"
#include "CanStatistics.h"
#include <sys/socket.h>
#include <linux/can.h>
#include "libsocketcan.h"

/*
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus access layer that will communicate with socket can (Linux only)
 */
using namespace CanModule;



class CSockCanScan : public CCanAccess
{
public:
	//Constructor of the class. Will initiate the statistics.
	CSockCanScan();
	//Disables copy constructor
	CSockCanScan(CSockCanScan const & other) = delete;
	//Disables asignation
	CSockCanScan& operator=( CSockCanScan const & other) = delete;
	//Destructor of the class
	virtual ~CSockCanScan();

	/*
	 * Method that sends a message trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
	 * can bus channel to send a message trough.
	 * @param cobID: Identifier that will be used for the message.
	 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
	 * @param message: Message to be sent trough the can bus.
	 * @param rtr: is the message a remote transmission request?
	 * @return: Was the initialisation process successful?
	 */
	virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);

	/*
	 * Method that sends a remote request trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
	 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
	 * @param cobID: Identifier that will be used for the request.
	 * @return: Was the initialisation process successful?
	 */
	virtual bool sendRemoteRequest(short cobID);

	/*
	 * Method that initialises a can bus channel. The following methods called upon the same object will be using this initialised channel.
	 *
	 * @param name: Name of the can bus channel. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
	 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
	 * @param parameters: Different parameters used for the initialisation. For using the default parameters just set this to "Unspecified"
	 * @return: Was the initialisation process successful?
	 */
	virtual bool createBus(const string name ,string parameters );

	//Returns socket handler
	int getHandler() { return m_sock; }
	//Returns channel name
	//   std::string &getChannel() { return m_channelName; }
	//Returns can bus parameters
	//    std::string &getParameters() { return m_parameters; }
	//Returns the instance of the CanStatistics object
	virtual void getStatistics( CanStatistics & result );
	static can_frame emptyCanFrame( void ){
		can_frame f;
		f.can_dlc = 0;
		f.can_id = 0;
		for ( int i = 0; i < 8; i++){
			f.data[ i ] = 0;
		}
		return(f);
	}
	static std::map<string, string> m_busMap;

private:

	//Flag for shutting down the CanScan thread
	volatile bool m_CanScanThreadShutdownFlag;
	//Socket handler
	int m_sock;
	//Instance of Can Statistics
	CanStatistics m_statistics;
	//Handle for the CAN update scan manager thread.
	pthread_t m_hCanScanThread;
	//Thread ID for the CAN update scan manager thread.
	int m_idCanScanThread;

	static Log::LogComponentHandle st_logItHandleSock;
	static bool st_logItRegisteredSock;

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

	//Channel name
	std::string m_channelName;

	//! As up-to-date as possible state of the interface.
	int m_errorCode;
};


#endif /* SOCKCANSCAN_H_ */
