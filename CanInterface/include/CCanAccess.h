/*
 * CCanAccess.h
 *
 *  Created on: Apr 4, 2011
 *      Author: vfilimon
 */

#ifndef CCANACCESS_H_
#define CCANACCESS_H_

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif
#include "boost/bind.hpp"
#include "boost/signals2.hpp"
#include <string>
#include "CanMessage.h"
#include "CanStatistics.h"
#include <LogIt.h>
/*
 * CCanAccess is an abstract class that defines the interface for controlling a canbus. Different implementations for different hardware and platforms should
 * inherit this class and implement the pure virtual methods.
 */
namespace CCC
{
	class CCanAccess
	{
	public:
		//Empty constructor
		CCanAccess() {};

		/*
		 * Method that initialises a can bus channel. The following methods called upon the same object will be using this initialised channel.
		 *
		 * @param name: Name of the can bus channel. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
		 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
		 * @param parameters: Different parameters used for the initialisation. For using the default parameters just set this to "Unspecified"
		 * @return: Was the initialisation process successful?
		 */
		virtual bool createBUS(const char *name, const char *parameters) = 0 ;

		/*
		 * Method that sends a remote request trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
		 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
		 * @param cobID: Identifier that will be used for the request.
		 * @return: Was the initialisation process successful?
		 */
		virtual bool sendRemoteRequest(short cobID) = 0;

		/*
		 * Method that sends a message trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
		 * can bus channel to send a message trough.
		 * @param cobID: Identifier that will be used for the message.
		 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
		 * @param message: Message to be sent trough the can bus.
		 * @param rtr: activate the Remote Transmission Request flag. Having this flag in a message with data/len is not part of the CAN standard,
		 * but since some hardware uses it anyway, the option to use it is provided.
		 * @return: Was the initialisation process successful?
		 */
		virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false) = 0;

		//Returns bus name
		std::string& getBusName() { return m_sBusName; }
		//Changes bus name
		void setBusName(std::string &name) {  m_sBusName = name;  }
		//Empty destructor
		virtual ~CCanAccess() {};

		/*
		 * Signal that will be called when a can Message arrives into the initialised can bus.
		 * In order to process this message manually, a handler needs to be connected to the signal.
		 *
		 * Example: myCCanAccessPointer->canMessageCame.connect(&myMessageRecievedHandler);
		 */
		boost::signals2::signal<void (const CanMessage &) > canMessageCame;

		/*
		 * Signal that will be called when a can Error arrives into the initialised can bus.
		 * In order to process this message manually, a handler needs to be connected to the signal.
		 *
		 * Example: myCCanAccessPointer->canMessageError.connect(&myErrorRecievedHandler);
		 */
		boost::signals2::signal<void (const int,const char *,timeval &) > canMessageError;
		//Returns the CanStatistics object.
		virtual void getStatistics( CanStatistics & result ) = 0;

		virtual bool initialiseLogging(LogItInstance* remoteInstance) = 0;

	private:
		std::string m_sBusName;
	};
};
#endif /* CCANACCESS_H_ */
