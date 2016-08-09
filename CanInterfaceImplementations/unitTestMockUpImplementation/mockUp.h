/*
 * stcan.h
 *
 *  Created on: Feb 22, 2012
 *      Author: vfilimon
 */

#ifndef MOCKUP_H_
#define MOCKUP_H_

#include <string>
#include "CanStatistics.h"
#include "CCanAccess.h"

#ifdef _WIN32

#include "tchar.h"
#include "Winsock2.h"
#include "windows.h"

#else

typedef unsigned long DWORD;

#endif
/*
 * This is an implementation of the abstract class CCanAccess. It serves as a can bus access layer that will communicate with Systec crates (Windows only)
 */
class mockUp: public CanModule::CCanAccess
{
public:
	//Constructor of the class. Will initiate the statistics.
	mockUp();
	//Disables copy constructor
	mockUp(mockUp const & other) = delete;
	//Disables asignation
	mockUp& operator=(mockUp const & other) = delete;
	//Destructor of the class
	virtual ~mockUp();

	/*
	 * Method that initialises a can bus channel. The following methods called upon the same object will be using this initialised channel.
	 *
	 * @param name: Name of the can bus channel. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
	 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
	 * @param parameters: Different parameters used for the initialisation. For using the default parameters just set this to "Unspecified"
	 * @return: Was the initialisation process successful?
	 */
	virtual bool createBUS(const char * name ,const char *parameters);

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

	void statisticsOnRecieve(int);
	void callbackOnRecieve(CanMessage&);

private:
	//The number of can handle associated with this instance.
	int m_canHandleNumber;
	//The number of can handle associated with this instance.
	const char*  m_canIPAddress;
	//Instance of the can handle
	int m_UcanHandle;
	//Instance of Can Statistics
	CanStatistics m_statistics;
	//Current baud rate
	unsigned int m_baudRate;

	std::string getCanName() { return m_canName; }
	bool sendErrorCode(int);

	//bool m_CanScanThreadShutdownFlag;
	//Name of the can channel
	std::string m_canName;
    // Handle for the CAN update scan manager thread.
    //HANDLE	m_hCanScanThread;
    // Thread ID for the CAN update scan manager thread.
    DWORD   m_idCanScanThread;


	int configureCanboard(const char *name,const char *parameters);
	/** Obtains a Systec canport and opens it.
	*  The name of the port and parameters should have been specified by preceding call to configureCanboard()
	*
	*  @returns less than zero in case of error, otherwise success
	*/
	int openCanPort(unsigned int operationMode, unsigned int bTermination, unsigned int bHighspeed, unsigned int bTimestamp);
	/*
	 * Provides textual representation of an error code.
	 */
	bool errorCodeToString(long error, char message[]);
	static void setCanHandleInUse(int n,bool t) { s_isCanHandleInUseArray[n] = t; }
	static bool isCanHandleInUse(int n) { return s_isCanHandleInUseArray[n]; }
	static void setCanHandle(int n,int tU) { s_canHandleArray[n] = tU; }
	static int getCanHandle(int n) { return s_canHandleArray[n]; }

	static int s_canHandleArray[256];
	static bool s_isCanHandleInUseArray[256];


};

#endif //MOCKUP_H_
