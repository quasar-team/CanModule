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
#include "usbcan32.h"
#include "CCanAccess.h"

using namespace CanModule;

/*
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus access layer that will communicate with Systec crates (Windows only)
 */
class STCanScan: public CanModule::CCanAccess
{
public:
	//Constructor of the class. Will initiate the statistics.
	STCanScan();
	//Disables copy constructor
	STCanScan(STCanScan const & other) = delete;
	//Disables asignation
	STCanScan& operator=(STCanScan const & other) = delete;
	//Destructor of the class
	virtual ~STCanScan();

	/*
	 * Method that initialises a can bus channel. The following methods called upon the same object will be using this initialised channel.
	 *
	 * @param name: Name of the can bus channel. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
	 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
	 * @param parameters: Different parameters used for the initialisation. For using the default parameters just set this to "Unspecified"
	 * @return: Was the initialisation process successful?
	 */
	virtual bool createBus(const string name ,const string parameters);

	/*
	 * Method that sends a message trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
	 * can bus channel to send a message trough.
	 * @param cobID: Identifier that will be used for the message.
	 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
	 * @param message: Message to be sent trough the can bus.
	 * @param rtr: is the message a remote transmission request?
	 * @return: Was the sending process successful?
	 */
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

private:
	//The number of the can module associated with this instance.
	int m_moduleNumber;
	//The number of the can channel associated with this instance.
	int m_channelNumber;
	//The number of can handle associated with this instance.
	int m_canHandleNumber;
	//Instance of the can handle
	tUcanHandle m_UcanHandle;
	//Instance of Can Statistics
	CanStatistics m_statistics;
	//Current baud rate
	unsigned int m_baudRate;

	static Log::LogComponentHandle s_logItHandleSt;
	static bool s_logItRegisteredSt;

	bool sendErrorCode(long);

	DWORD	m_busStatus;
	bool m_CanScanThreadShutdownFlag;
    // Handle for the CAN update scan manager thread.
    HANDLE	m_hCanScanThread;
    // Thread ID for the CAN update scan manager thread.
    DWORD   m_idCanScanThread;
    // The main control thread function for the CAN update scan manager.
	static DWORD WINAPI CanScanControlThread(LPVOID pCanScan);

	int configureCanBoard(const string name,const string parameters);
	/** Obtains a Systec canport and opens it.
	*  The name of the port and parameters should have been specified by preceding call to configureCanboard()
	*
	*  @returns less than zero in case of error, otherwise success
	*/
	int openCanPort(tUcanInitCanParam initializationParameters);
	/*
	 * Provides textual representation of an error code.
	 */
	bool errorCodeToString(long error, char message[]);
	static void setCanHandleInUse(int n,bool t) { s_isCanHandleInUseArray[n] = t; }
	static bool isCanHandleInUse(int n) { return s_isCanHandleInUseArray[n]; }
	static void setCanHandle(int n,tUcanHandle tU) { s_canHandleArray[n] = tU; }
	static tUcanHandle getCanHandle(int n) { return s_canHandleArray[n]; }

	static tUcanHandle s_canHandleArray[256];
	static bool s_isCanHandleInUseArray[256];


};

#endif
