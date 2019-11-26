/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CCanAccess.h
 *
 *  Created on: Apr 4, 2011
 *      Author: vfilimon
 *      maintaining touches: mludwig, quasar team
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
 *
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
#include "VERSION.h"
#include <LogIt.h>



/*
 * CCanAccess is an abstract class that defines the interface for controlling a canbus. Different implementations for different hardware and platforms should
 * inherit this class and implement the pure virtual methods.
 */
namespace CanModule
{

const std::string LogItComponentName = "CanModule";
#define MLOG(LEVEL,THIS) LOG(Log::LEVEL) << __FUNCTION__ << " " << CanModule::LogItComponentName << " bus= " << THIS->getBusName() << " "

static std::string version(){ return( CanModule_VERSION ); }

struct CanParameters {
	long m_lBaudRate;
	unsigned int m_iOperationMode;
	unsigned int m_iTermination;
	unsigned int m_iHighSpeed;
	unsigned int m_iTimeStamp;
	int	m_iNumberOfDetectedParameters;
	bool m_dontReconfigure;
	CanParameters() : m_lBaudRate(0), m_iOperationMode(0), m_iTermination(0),
			m_iHighSpeed(1), m_iTimeStamp(0), m_iNumberOfDetectedParameters(), m_dontReconfigure(false) {}

	void scanParameters(string parameters)
	{
		const char * canpars = parameters.c_str();
		if (strcmp(canpars, "Unspecified") != 0) {
#ifdef _WIN32
			m_iNumberOfDetectedParameters = sscanf_s(canpars, "%ld %u %u %u %u", &m_lBaudRate, &m_iOperationMode, &m_iTermination, &m_iHighSpeed, &m_iTimeStamp);
#else
			m_iNumberOfDetectedParameters = sscanf(canpars, "%ld %u %u %u %u", &m_lBaudRate, &m_iOperationMode, &m_iTermination, &m_iHighSpeed, &m_iTimeStamp);
#endif
		} else {
			m_dontReconfigure = true;
		}

	}
};

class CCanAccess
{

public:
	//Empty constructor, just get rid of a useless warning
	CCanAccess():
		s_connectionIndex(0),
		s_logItRemoteInstance(NULL)
	{
		CanModule::version();
	};



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


	/*
	 * Signal that will be called when a can Message arrives into the initialised can bus.
	 * In order to process this message manually, a handler needs to be connected to the signal.
	 *
	 * Example: myCCanAccessPointer->canMessageCame.connect(&myMessageRecievedHandler);
	 */
	boost::signals2::signal<void (const CanMessage &) > canMessageCame;

	virtual ~CCanAccess() {};


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
		s_logItRemoteInstance = remoteInstance;
		return ret;
	}

	/**
	 * the LogIt instance is NOT shared by inheritance in windows, the instance has to be passed explicitly
	 * from the parent
	 */
	LogItInstance* getLogItInstance()
	{
		return( s_logItRemoteInstance );
	}

	/* @ Parse the input parameters
	 * @param name The parameters have a format <name component>:name chanel:options for add address parameters>
	 * @param parameters is a string with possible 6 word describing can options
	 * @return: the result is saved in internal variable m_sBusName and m_CanParameters
	 */
	inline vector<string> parseNameAndParameters(string name, string parameters){
		LOG(Log::TRC, _lh) << __FUNCTION__ << " name= " << name << " parameters= " << parameters;

		bool isSocketCanLinux = false;
		std::size_t s = name.find("sock");
		if ( s != std::string::npos ){
			isSocketCanLinux = true;
		}

		// care for ":vcanwhatsoever0:" for linux. It shall become ":vcan0"
		std::size_t foundVcan = name.find_first_of (":vcan", 0);

		// strip off any leading chars from port number: ":(v)canwhatever7:" becomes just ":7:"
		std::size_t foundCan = name.find_first_of (":", 0);
		std::size_t found1 = name.find_first_of ( "0123456789", foundCan );
		if ( found1 != std::string::npos ) name.erase( foundCan + 1, found1 - foundCan - 1 );

		// for socketcan, have to prefix "can" or "vcan" to port number
		if ( isSocketCanLinux ){
			foundCan = name.find_first_of (":", 0);
			if ( foundVcan != std::string::npos ) {
				m_sBusName = name.insert( foundCan + 1, "vcan");
			} else {
				m_sBusName = name.insert( foundCan + 1, "can");
			}
		} else {
			m_sBusName = name;
		}

		LOG(Log::TRC, _lh) << __FUNCTION__ << " m_sBusName= " << m_sBusName;

		vector<string> stringVector;
		istringstream nameSS(name);
		string temporalString;
		while (getline(nameSS, temporalString, ':')) {
			stringVector.push_back(temporalString);
			LOG(Log::TRC, _lh) << __FUNCTION__ << " stringVector new element= " << temporalString;
		}
		m_CanParameters.scanParameters(parameters);
		LOG(Log::TRC, _lh) << __FUNCTION__ << " stringVector size= " << stringVector.size();
		return stringVector;
	}

protected:
	string m_sBusName;
	CanParameters m_CanParameters;

private:

	boost::signals2::connection s_cconnection;
	int s_connectionIndex;
	Log::LogComponentHandle _lh; // s_lh ?!? @ windows w.t.f.
	LogItInstance* s_logItRemoteInstance;

};
};
#endif /* CCANACCESS_H_ */
