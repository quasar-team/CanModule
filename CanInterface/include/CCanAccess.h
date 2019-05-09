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
#include <LogIt.h>


#define VERSION "CanModule version 1.0.3"

/*
 * CCanAccess is an abstract class that defines the interface for controlling a canbus. Different implementations for different hardware and platforms should
 * inherit this class and implement the pure virtual methods.
 */
namespace CanModule
{

const std::string LogItComponentName = "CanModule";
const std::string LogItComponentNameAnagate = LogItComponentName + "Anagate";
#ifdef _WIN32
const std::string LogItComponentNamePeak = LogItComponentName + "Peak";
const std::string LogItComponentNameSystec = LogItComponentName + "Systec";
#else
/**
 * linux only, we can't distinguish between peak and systec on the socket level,
 * but if the user wants to use Peak or Systec she may
 */
const std::string LogItComponentNameSock = LogItComponentName + "Sock";
const std::string LogItComponentNamePeak = LogItComponentNameSock;
const std::string LogItComponentNameSystec = LogItComponentNameSock;
#endif

#define MLOG(LEVEL,THIS) LOG(Log::LEVEL) << __FUNCTION__ << " " << CanModule::LogItComponentName << " bus= " << THIS->getBusName() << " "

static std::string version(){ return( VERSION ); }

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
		if (strcmp(canpars, "Unspecified") != 0) {
#ifdef _WIN32
			m_iNumberOfDetectedParameters = sscanf_s(canpars, "%ld %u %u %u %u %u", &m_lBaudRate, &m_iOperationMode, &m_iTermination, &m_iHighSpeed, &m_iTimeStamp, &m_iSyncMode);
#else
			m_iNumberOfDetectedParameters = sscanf(canpars, "%ld %u %u %u %u %u", &m_lBaudRate, &m_iOperationMode, &m_iTermination, &m_iHighSpeed, &m_iTimeStamp, &m_iSyncMode);
#endif
		} else {
			m_dontReconfigure = true;
		}

	}
};

class CCanAccess
{

public:
	static boost::function< void ( const CanMsgStruct ) > fw_slot0;
	static boost::function< void ( const CanMsgStruct ) > fw_slot1;
	static boost::function< void ( const CanMsgStruct ) > fw_slot2;
	static boost::function< void ( const CanMsgStruct ) > fw_slot3;
	static boost::function< void ( const CanMsgStruct ) > fw_slot4;
	static boost::function< void ( const CanMsgStruct ) > fw_slot5;
	static boost::function< void ( const CanMsgStruct ) > fw_slot6;
	static boost::function< void ( const CanMsgStruct ) > fw_slot7;
	static boost::function< void ( const CanMsgStruct ) > fw_slot8;
	static boost::function< void ( const CanMsgStruct ) > fw_slot9;
	static boost::function< void ( const CanMsgStruct ) > fw_slot10;
	static boost::function< void ( const CanMsgStruct ) > fw_slot11;
	static boost::function< void ( const CanMsgStruct ) > fw_slot12;
	static boost::function< void ( const CanMsgStruct ) > fw_slot13;
	static boost::function< void ( const CanMsgStruct ) > fw_slot14;
	static boost::function< void ( const CanMsgStruct ) > fw_slot15;


#ifdef _WIN32
	LogItInstance* myRemoteInstance;
#endif

	//Empty constructor, just get rid of a useless warning
	CCanAccess():
		_connectionIndex(0)
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

	/** call an external user-code function to treat the can message. Use boost function
	 * wrappers for this. All these are static.
	 */
	static void connectFwSlotX( int connectionIndex, boost::function<void(const CanMsgStruct)> userFunction ){
		// LOG(Log::TRC) << " connecting user handler to slot" <<  connectionIndex << endl;
		switch ( connectionIndex ){
		case 0: { fw_slot0 = userFunction; break; }
		case 1: { fw_slot1 = userFunction; break; }
		case 2: { fw_slot2 = userFunction; break; }
		case 3: { fw_slot3 = userFunction; break; }
		case 4: { fw_slot4 = userFunction; break; }
		case 5: { fw_slot5 = userFunction; break; }
		case 6: { fw_slot6 = userFunction; break; }
		case 7: { fw_slot7 = userFunction; break; }
		case 8: { fw_slot8 = userFunction; break; }
		case 9: { fw_slot9 = userFunction; break; }
		case 10: { fw_slot10 = userFunction; break; }
		case 11: { fw_slot11 = userFunction; break; }
		case 12: { fw_slot12 = userFunction; break; }
		case 13: { fw_slot13 = userFunction; break; }
		case 14: { fw_slot14 = userFunction; break; }
		case 15: { fw_slot15 = userFunction; break; }
		default: {
			LOG(Log::ERR) << __FUNCTION__ << " " << CanModule::LogItComponentName << " can not connect user handler to slot" <<  connectionIndex;
		}
		}
	}
	static void slot0( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot0 invoked, calling user handler";
		fw_slot0( msg );
	}
	static void slot1( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot1 invoked, calling user handler";
		fw_slot1( msg );
	}
	static void slot2( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot2 invoked, calling user handler";
		fw_slot2( msg );
	}
	static void slot3( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot3 invoked, calling user handler";
		fw_slot4( msg );
	}
	static void slot4( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot4 invoked, calling user handler";
		fw_slot4( msg );
	}
	static void slot5( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot5 invoked, calling user handler";
		fw_slot5( msg );
	}
	static void slot6( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot6 invoked, calling user handler";
		fw_slot6( msg );
	}
	static void slot7( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot7 invoked, calling user handler";
		fw_slot7( msg );
	}
	static void slot8( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot8 invoked, calling user handler";
		fw_slot8( msg );
	}
	static void slot9( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot9 invoked, calling user handler";
		fw_slot9( msg );
	}
	static void slot10( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot10 invoked, calling user handler";
		fw_slot10( msg );
	}
	static void slot11( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot11 invoked, calling user handler";
		fw_slot11( msg );
	}
	static void slot12( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot12 invoked, calling user handler";
		fw_slot12( msg );
	}
	static void slot13( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot13 invoked, calling user handler";
		fw_slot13( msg );
	}
	static void slot14( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot14 invoked, calling user handler";
		fw_slot14( msg );
	}
	static void slot15( const CanMsgStruct msg){
		// LOG(Log::TRC) << " slot15 invoked, calling user handler";
		fw_slot15( msg );
	}

	void testSignalSlot( void ){
		LOG(Log::TRC, lh) << " sending a test signal to slot" << endl;
		CanMessage cm;
		canMessageCame( cm );
		LOG(Log::TRC, lh) << " a test signal to this object's slot was sent" << endl;
	}

	void connectReceptionSlotX( int connectionIndex )
	{
		LOG(Log::TRC, lh) << " connecting internal slot to boost signal of this connection " << connectionIndex;
		if ( _cconnection.connected() ){
			LOG(Log::WRN, lh) << "internal slot is already connected, disconnecting";
			_cconnection.disconnect();
		}
		switch( connectionIndex ){
		case 0:{ _cconnection = canMessageCame.connect( &slot0 ); break; }
		case 1:{ _cconnection = canMessageCame.connect( &slot1 ); break; }
		case 2:{ _cconnection = canMessageCame.connect( &slot2 ); break; }
		case 3:{ _cconnection = canMessageCame.connect( &slot3 ); break; }
		case 4:{ _cconnection = canMessageCame.connect( &slot4 ); break; }
		case 5:{ _cconnection = canMessageCame.connect( &slot5 ); break; }
		case 6:{ _cconnection = canMessageCame.connect( &slot6 ); break; }
		case 7:{ _cconnection = canMessageCame.connect( &slot7 ); break; }
		case 8:{ _cconnection = canMessageCame.connect( &slot8 ); break; }
		case 9:{ _cconnection = canMessageCame.connect( &slot9 ); break; }
		case 10:{ _cconnection = canMessageCame.connect( &slot10 ); break; }
		case 11:{ _cconnection = canMessageCame.connect( &slot11 ); break; }
		case 12:{ _cconnection = canMessageCame.connect( &slot12 ); break; }
		case 13:{ _cconnection = canMessageCame.connect( &slot13 ); break; }
		case 14:{ _cconnection = canMessageCame.connect( &slot14 ); break; }
		case 15:{ _cconnection = canMessageCame.connect( &slot15 ); break; }
		default: {
			LOG(Log::ERR) << "can not connect to internal slot " << connectionIndex << " (available slots 0..15)"; }
		}
		_connectionIndex = connectionIndex;
		LOG(Log::INF) << "OK connected internal slot" << _connectionIndex << " to boost signal of this object";
	}
	void disconnectReceptionSlotX( void )
	{
		LOG(Log::TRC, lh) << __FUNCTION__ << " disconnecting internal slot " << _connectionIndex;
		canMessageCame.disconnect( _cconnection );
	}

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
//#ifdef _WIN32
		myRemoteInstance = remoteInstance;
//#endif
		return ret;
	}

//#ifdef _WIN32
	/**
	 * the LogIt instance is NOT shared by inheritance in windows, the instance has to be passed explicitly
	 * from the parent
	 */
	LogItInstance* getLogItInstance()
	{
		return( myRemoteInstance );
	}
//#endif

	/* @ Parse the input parameters
	 * @param name The parameters have a format <name component>:name chanel:options for add address parameters>
	 * @param parameters is a string with possible 6 word describing can options
	 * @return: the result is saved in internal variable m_sBusName and m_CanParameters
	 */
	inline vector<string> parseNameAndParameters(string name, string parameters){
		LOG(Log::TRC, lh) << __FUNCTION__ << " name= " << name << " parameters= " << parameters;

		m_sBusName = name;
		vector<string> stringVector;
		istringstream nameSS(name);
		string temporalString;
		while (getline(nameSS, temporalString, ':')) {
			stringVector.push_back(temporalString);
			LOG(Log::TRC, lh) << __FUNCTION__ << " stringVector new element= " << temporalString;
		}
		m_CanParameters.scanParameters(parameters);
		LOG(Log::TRC, lh) << __FUNCTION__ << " stringVector size= " << stringVector.size();
		return stringVector;
	}

protected:
	string m_sBusName;
	CanParameters m_CanParameters;

private:
	boost::signals2::connection _cconnection;
	int _connectionIndex;
	Log::LogComponentHandle lh;

};
};
#endif /* CCANACCESS_H_ */
