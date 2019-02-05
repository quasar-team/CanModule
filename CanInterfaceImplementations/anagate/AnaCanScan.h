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

#include <boost/thread/thread.hpp> // just needed for the boost sleep

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
	//Constructor of the class. Will initiate the statistics.
	AnaCanScan();
	//Disables copy constructor
	AnaCanScan(AnaCanScan const & other) = delete;
	//Disables asignation
	AnaCanScan& operator=(AnaCanScan const & other) = delete;
	//Destructor of the class
	virtual ~AnaCanScan();

	/**
	 * Method that initialises a can bus channel. The following methods called upon the same object will be using this initialised channel.
	 *
	 * @param name: Name of the can bus channel. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
	 * 				16 port systec interface usb1 would be using name "st:9", while in socket can the same channel would be "sock:can0".
	 * 				for anagate, this is a ":" separated string with three elements, example: "an:0:128.141.159.194"
	 * 				with:
	 * 				 "an" = anagate library
	 * 				 0 = the physical module port (if multiport), 0=A, 1=B etc etc
	 * 				 a.b.c.d = the IP number (here, out lab module)
	 *
	 *
	 * @param parameters: Different parameters used for the initialisation. For using the default parameters just set this to "Unspecified"
	 *
	 *              for anagate we have a string of 6 parameters separated by whitespace, i.e. "125000 0 0 0 0 0"
	 *
	 *  				m_iNumberOfDetectedParameters = sscanf(canpars, "%ld %u %u %u %u %u", &m_lBaudRate, &m_iOperationMode, &m_iTermination, &m_iHighSpeed, &m_iTimeStamp, &m_iSyncMode);
	 *
	 *              value0 = baud rate, default 125000
	 *              value1 = operationMode, default 0
	 *              value2 = termination, default 0
	 *              value3 = high speed, default 0
	 *              value4 = time stamp, default 0
	 *              value5 = sync mode, default 0
	 *              (see anagate manual for explanation what this actually all means)
	* i.e. "250000 0 1 0 0 1"
	 * @return: Was the initialisation process successful?
	 */
	virtual bool createBus(const string name ,const string parameters);

	/**
	 * we throttle the message sending, by introducing a usleep between successive sends
	 */
	void setMessageDelay( int usleep );

	/**
	 * Method that sends a message trough the can bus channel. If the method createBus was not called before this, sendMessage will fail, as there is no
	 * can bus channel to send a message trough.
	 * @param cobID: Identifier that will be used for the message.
	 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
	 * @param message: Message to be sent trough the can bus.
	 * @param rtr: is the message a remote transmission request?
	 * @return: Was the sending process successful?
	 */
    virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false);
	virtual bool sendMessage(CanMessage *);

    /**
	 * Method that sends a remote request trough the can bus channel. If the method createBus was not called before this, sendMessage will fail, as there is no
	 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
	 * @param cobID: Identifier that will be used for the request.
	 * @return: Was the initialisation process successful?
	 */
	virtual bool sendRemoteRequest(short cobID);

	virtual void getStatistics( CanStatistics & result );
	void statisticsOnRecieve(int);
	void callbackOnRecieve(CanMessage&);
	void startAlive( int aliveTime_sec );
	void setConnectWaitTime( int timeout_ms );

private:

	int m_canPortNumber; //The number of can port (CANA, CANB, ...) associated with this instance.
	string  m_canIPAddress;
	AnaInt32 m_UcanHandle; //Instance of the can handle
	CanStatistics m_statistics; //Instance of Can Statistics
	unsigned int m_baudRate; 	//Current baud rate for statistics
    DWORD   m_idCanScanThread; // Thread ID for the CAN update scan manager thread.
	bool m_canCloseDevice;
	string m_busName;
	string m_busParameters;
    AnaInt32 m_timeout; 		// connect_wait time

    bool sendErrorCode(AnaInt32);

	int configureCanBoard(const string name,const string parameters);

	/**
	 * Obtains a Systec canport and opens it.
	 *  The name of the port and parameters should have been specified by preceding call to configureCanboard()
	 *  @returns less than zero in case of error, otherwise success
	 */
	int openCanPort();

	/**
	 * we try to reconnect on the same handle with the same parameters
	 * OK = 0
	 * no success = 1
	 */
	int reconnect();

	/**
	 * Provides textual representation of an error code.
	 */
	bool errorCodeToString(long error, char message[]);
	static void setCanHandleInUse(int n,bool t) { s_isCanHandleInUseArray[n] = t; }
	static bool isCanHandleInUse(int n) { return s_isCanHandleInUseArray[n]; }
	static void setCanHandle(int n,AnaInt32 tU) { s_canHandleArray[n] = tU; }
	static AnaInt32 getCanHandle(int n) { return s_canHandleArray[n]; }

	static AnaInt32 s_canHandleArray[256];
	static bool s_isCanHandleInUseArray[256];

};

#endif //CCANANASCAN_H_
