/*
 *pkcan.h
 *
 *  Created on: JUL 2, 2012
 *      Author: vfilimon
 */

#ifndef CCANPKSCAN_H_
#define CCANPKSCAN_H_


#include <string>
#include "Winsock2.h"
#include "windows.h"

#include <PCANBasic.h>
#include "CCanAccess.h"
#include "CanStatistics.h"

/*
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus access layer that will communicate with Peak USB-CAN devices (Windows only)
 */
class PKCanScan: public CanModule::CCanAccess
{
public:
	//Constructor of the class. Will initiate the statistics.
	PKCanScan();
	//Disables copy constructor
	PKCanScan(PKCanScan const & other) = delete;
	//Disables asignation
	PKCanScan& operator=(PKCanScan const & other) = delete;
	//Destructor of the class
	virtual ~PKCanScan();
	/*
	 * Method that initialises a can bus channel. The following methods called upon the same object will be using this initialised channel.
	 *
	 * @param name: Name of the can bus channel. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
	 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
	 * @param parameters: Different parameters used for the initialisation. For using the default parameters just set this to "Unspecified"
	 * @return: Was the initialisation process successful?
	 */
	virtual bool createBUS(const char * name,const char * parameters);
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

	virtual bool initialiseLogging(LogItInstance* remoteInstance);
	/*
	 * Converts Error code into a text message.
	 */
	bool getErrorMessage(long error, char **message);

private:
	TPCANHandle getHandle(const char *name);
	std::string getCanName() { return m_canName; }
	bool sendErrorCode(long);
	// The main control thread function for the CAN update scan manager.
	static DWORD WINAPI CanScanControlThread(LPVOID pCanScan);

	//Instance of Can Statistics
	CanStatistics m_statistics;
	TPCANStatus m_busStatus;
	bool m_CanScanThreadShutdownFlag;
	//Current baud rate
	unsigned int m_baudRate;

	//Name of the can channel
	std::string m_canName;
	//Instance of the can handle
   	TPCANHandle	m_canObjHandler;
	bool configureCanboard(const char *name,const char *parameters);

	HANDLE      m_hCanScanThread;
	HANDLE		m_ReadEvent;

    // Thread ID for the CAN update scan manager thread.
    DWORD           m_idCanScanThread;

};

#endif
