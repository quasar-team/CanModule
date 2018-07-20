/*
 * CCanAccess.h
 *
 *  Created on: Apr 4, 2011
 *      Author: vfilimon
 *      maintaining touches: mludwig
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
namespace CanModule
{
	#define MLOG(LEVEL,THIS) LOG(Log::LEVEL) << THIS->getBusName() << " "

	struct CanParameters {
		long m_lBaudRate;
		unsigned int m_iOperationMode;
		unsigned int m_iTermination;
		unsigned int m_iHighSpeed;
		unsigned int m_iTimeStamp;
		unsigned int m_iSyncMode;
		int	m_iNumberOfDetectedParameters;
           bool m_dontReconfigure;
		CanParameters() : m_lBaudRate(0), m_iOperationMode(0), m_iTermination(0), 
			m_iHighSpeed(0), m_iTimeStamp(0), m_iSyncMode(0), m_iNumberOfDetectedParameters(), m_dontReconfigure(false) {}

		void scanParameters(string parameters)
		{
			const char * canpars = parameters.c_str();

			if (strcmp(canpars, "Unspecified") != 0)
			{
#ifdef _WIN32
				m_iNumberOfDetectedParameters = sscanf_s(canpars, "%ld %u %u %u %u %u", &m_lBaudRate, &m_iOperationMode, &m_iTermination, &m_iHighSpeed, &m_iTimeStamp, &m_iSyncMode);
#else
				m_iNumberOfDetectedParameters = sscanf(canpars, "%ld %u %u %u %u %u", &m_lBaudRate, &m_iOperationMode, &m_iTermination, &m_iHighSpeed, &m_iTimeStamp, &m_iSyncMode);
#endif
			}
                 else
                 {
            		m_dontReconfigure = true;
                 }

		}
	};

	class CCanAccess
	{
	public:
		//Empty constructor
		CCanAccess() {};

		/*
		 * Method that sends a remote request trough the can bus channel. If the method createBus was not called before this, sendMessage will fail, as there is no
		 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
		 * @param cobID: Identifier that will be used for the request.
		 * @return: Was the initialisation process successful?
		 */
		virtual bool sendRemoteRequest(short cobID) = 0;


		/*
		 * Method that initialises a can bus channel. The following methods called upon the same object will be using this initialised channel.
		 *
		 * @param name: Name of the can bus channel. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
		 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
		 * 				anagate interface would be an:192.168.1.2 for the default ip address
		 * @param parameters: Different parameters used for the initialisation. For using the default parameters just set this to "Unspecified"
		 * 				anagate: pass the ip number
		 * @return: Was the initialisation process successful?
		 */
		virtual bool createBus(const string name, const string parameters) = 0;

		/*
		 * Method that sends a message through the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
		 * can bus channel to send a message through.
		 * @param cobID: Identifier that will be used for the message.
		 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
		 * @param message: Message to be sent through the can bus.
		 * @param rtr: activate the Remote Transmission Request flag. Having this flag in a message with data/len is not part of the CAN standard,
		 * but since some hardware uses it anyway, the option to use it is provided.
		 * @return: Was the initialisation process successful?
		 */
		virtual bool sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr = false) = 0;

		virtual bool sendMessage(CanMessage *canm)
		{
			return sendMessage(short(canm->c_id), canm->c_dlc, canm->c_data, canm->c_rtr);
		}


		//Returns bus name
		std::string& getBusName() { return m_sBusName; }
		//Changes bus name

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

		inline bool initialiseLogging(LogItInstance* remoteInstance)
		{
			bool ret = Log::initializeDllLogging(remoteInstance);
			LOG(Log::INF) << "DLL: logging initialised";
			return ret;
		}

		/* @ Parcer the input parameters  
		* @param name Thei parameters has a format <name componenet>:name chanel:options for add addres parameters>
		* @param parameters is a string with possable 6 word discribing can options
		* @return: the result is saved in internal variable m_sBusName and m_CanParameterts
		*/
		inline vector<string> parcerNameAndPar(string name, string parameters)
		{
			LOG(Log::TRC) << "CCanAccess::parcerNameAndPar name= " << name << " parameters= " << parameters;

			m_sBusName = name;
			vector<string> stringVector;
			istringstream nameSS(name);
			string temporalString;
			while (getline(nameSS, temporalString, ':')) {
				stringVector.push_back(temporalString);
				LOG(Log::TRC) << "CCanAccess::parcerNameAndPar: stringVector new element= " << temporalString;
			}
			m_CanParameters.scanParameters(parameters);
			LOG(Log::TRC) << "CCanAccess::parcerNameAndPar: stringVector size= " << stringVector.size();

			return stringVector;
		}

	protected:
		string m_sBusName;
		CanParameters m_CanParameters;
	};
};
#endif /* CCANACCESS_H_ */
