/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanLibLoader.cpp
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
#include "CanLibLoader.h"
#include "LogIt.h"
#include <string>
#include <stdexcept>
#ifdef _WIN32
#include "CanLibLoaderWin.h"
#else
#include "CanLibLoaderLin.h"
#endif

#include <Diag.h>

namespace CanModule
{

/* static */ GlobalErrorSignaler *GlobalErrorSignaler::instancePtr = NULL;
/* static */ LogItInstance * GlobalErrorSignaler::m_st_logIt = NULL;
/* static */ Log::LogComponentHandle GlobalErrorSignaler::m_st_lh = 0;

GlobalErrorSignaler::~GlobalErrorSignaler(){
	globalErrorSignal.disconnect_all_slots();
	LOG( Log::TRC, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " disconnected all handlers from signal.";
}

/**
 * singleton fabricator. We have one global signal only which is neither lib/vendor nor port specific, per task.
 */
GlobalErrorSignaler* GlobalErrorSignaler::getInstance() {
	if ( GlobalErrorSignaler::instancePtr == NULL) {
		GlobalErrorSignaler::instancePtr = new GlobalErrorSignaler();

		bool ret = Log::initializeLogging(Log::TRC);
		if ( ret ) {
			std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " LogIt initialized OK" << std::endl;
			LogItInstance *logIt = LogItInstance::getInstance();
			Log::LogComponentHandle lh = 0;
			if ( logIt != NULL ){
				logIt->getComponentHandle( CanModule::LogItComponentName, lh );
				LOG(Log::TRC, lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << "created singleton instance of GlobalErrorSignaler";

				GlobalErrorSignaler::m_st_logIt = logIt;
				GlobalErrorSignaler::m_st_lh = lh;
			} else {
				std::cout << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " logIt instance is NULL" << std::endl;
			}
		} else {
			std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " LogIt problem at initialisation" << std::endl;
		}
	}
	return GlobalErrorSignaler::instancePtr;
}


/**
 * connect/disconnect the handler provided by "the user": void myHandler( int code, const char *myMessage )
 *  connect/disconnect a given handler to the signal of the singleton. function pointer is the argument.
 *  Can have as many connected handlers as you want, but they will all be disconnected when the object goes out of scope (dtor).
 */
bool GlobalErrorSignaler::connectHandler( void (*fcnPtr)( int, const char*, timeval ) ){
	if ( fcnPtr != NULL ){
		globalErrorSignal.connect( fcnPtr );
		LOG( Log::INF, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " connect handler to signal.";
		return true;
	} else {
		LOG( Log::ERR, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " cannot connect NULL handler to signal. skipping...";
		return false;
	}
}
bool GlobalErrorSignaler::disconnectHandler( void (*fcnPtr)( int, const char*, timeval ) ){
	if ( fcnPtr != NULL ){
		globalErrorSignal.disconnect( fcnPtr );
		LOG( Log::INF, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " disconnect handler from signal.";
		return true;
	} else {
		LOG( Log::ERR, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " cannot disconnect NULL handler to signal. skipping...";
		return false;
	}
}
void GlobalErrorSignaler::disconnectAllHandlers() {
		globalErrorSignal.disconnect_all_slots();
		LOG( Log::INF, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " disconnected all handlers from signal.";
}
// fire the signal with payload. Timestamp done internally
void GlobalErrorSignaler::fireSignal( const int code, const char *msg ){
	timeval ftTimeStamp;
	auto now = std::chrono::system_clock::now();
	auto nMicrosecs = std::chrono::duration_cast<std::chrono::microseconds>( now.time_since_epoch());
	ftTimeStamp.tv_sec = nMicrosecs.count() / 1000000L;
	ftTimeStamp.tv_usec = (nMicrosecs.count() % 1000000L) ;
	globalErrorSignal( code, msg, ftTimeStamp );
}


// called by factory
CanLibLoader::CanLibLoader(const std::string& libName)
{
	m_gsig = GlobalErrorSignaler::getInstance();
	bool ret = Log::initializeLogging(Log::TRC);
	if (ret) {
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " LogIt initialized OK" << std::endl;
	} else {
		std::stringstream msg;
		msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " LogIt problem at initialisation";
		std::cout << msg.str() << std::endl;
		m_gsig->fireSignal( 001, msg.str().c_str() );
	}
	LogItInstance *logIt = LogItInstance::getInstance();
	if ( logIt != NULL ){
		logIt->getComponentHandle( CanModule::LogItComponentName, lh );
		std::cout << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " constructor " << libName << std::endl;
	} else {
		std::stringstream msg;
		msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " LogIt instance is NULL";
		std::cout << msg.str() << std::endl;
		m_gsig->fireSignal( 002, msg.str().c_str() );
	}
}


CanLibLoader::~CanLibLoader() {}

CanLibLoader* CanLibLoader::createInstance(const std::string& libName)	{
	CanLibLoader* libPtr = 0;
#ifdef WIN32
	std::cout << __FUNCTION__ << " calling CanLibLoaderWin " << libName << std::endl;
	libPtr = new CanLibLoaderWin(libName);
#else
	std::cout << __FUNCTION__ << " calling CanLibLoaderLin " << libName << std::endl;
	libPtr = new CanLibLoaderLin(libName);
#endif
	return libPtr;
}

/**
 * close a CAN bus
 */
void CanLibLoader::closeCanBus(CCanAccess *cInter) {
	LOG(Log::TRC, lh ) << __FUNCTION__;
	if ( cInter ) {
		LOG(Log::DBG, lh ) << __FUNCTION__<< " CanBus name to be deleted: " << cInter->getBusName();
		Diag::delete_maps( this, cInter );
		cInter->stopBus(); // call each specific stopBus, but dtors are left until objects are out of scope
	}
}

/**
 * opens a can bus
 */
CCanAccess* CanLibLoader::openCanBus(std::string name, std::string parameters) {
	LOG(Log::DBG, lh ) << __FUNCTION__ << " Creating CCanAccess: name= " << name << " parameters= " << parameters;
	CCanAccess *tcca = createCanAccess();

	if ( !tcca ){
		std::stringstream msg;
		msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " failed createCanAccess name= " << name << " parameters= " << parameters;
		LOG(Log::ERR, lh ) << msg.str();
		m_gsig->fireSignal( 003, msg.str().c_str() );

		throw std::runtime_error("CanLibLoader::openCanBus: createBus Problem when loading lib " + name );
	} else {
		LOG(Log::DBG, lh ) << __FUNCTION__ << " created CCanAccess name= " << name << " parameters= " << parameters;
	}

	//The Logit instance of the executable is handled to the DLL at this point, so the instance is shared.
	LogItInstance *logInstance = LogItInstance::getInstance() ;
	tcca->initialiseLogging( logInstance );
	logInstance->registerLoggingComponent( CanModule::LogItComponentName, Log::TRC );

	LOG(Log::DBG, lh ) << __FUNCTION__ << " calling createBus. name= " << name << " parameters= " << parameters;
	/** @param name: Name of the can bus channel. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
	 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
	 * 				anagate interface would be "an:0:192.168.1.2" for port A and ip address
	 * @param parameters: Different parameters used for the initialisation. For using the parameters present in the HW just set this to "Unspecified"
	 *
	 */
	int c = tcca->createBus(name, parameters);
	LOG(Log::DBG, lh ) << __FUNCTION__ << " createBus returns= " << c;
	switch ( c ){
	case 0:{
		LOG(Log::DBG, lh ) << __FUNCTION__ << " OK: createBus Adding new CCanAccess to the map for: " << name;
		Diag::insert_maps( this, tcca, parameters );
		return tcca;
		break;
	}
	case 1:{
		LOG(Log::DBG, lh ) << __FUNCTION__ << " OK: createBus Skipping existing CCanAccess to the map for: " << name;
		return tcca; // keep lib object, but only the already existing bus
		break;
	}
	case -1:{

		std::stringstream msg;
		msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " Problem opening canBus for: " << name;
		LOG(Log::ERR, lh ) << msg.str();
		m_gsig->fireSignal( 004, msg.str().c_str() );

		/** we could do 3 things here:
		 * 1. throw an exception and stop everything
		 *   throw std::runtime_error("CanLibLoader::openCanBus: createBus Problem when opening canBus. stop." );
		 * 2. try again looping
		 *   this would go on forever if the bus does not come up
		 * 3. ignore the bus in the map
		 *   we return the can access nevertheless in this case and the client has to check the pointer of course
		 *   we decided to do that 6.april.2021 quasar meeting, related to https://its.cern.ch/jira/browse/OPCUA-2248
		 */
		return tcca;
		break;
	}
	default:{
		std::stringstream msg;
		msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " something else went wrong for: " << name << " : returning NULL";
		LOG(Log::ERR, lh ) << msg.str();
		m_gsig->fireSignal( 005, msg.str().c_str() );
		return NULL;
		break;
	}
	} // switch

	// never reached ;-)
	std::stringstream msg;
	msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " something else went seriously wrong for: " << name << " : returning NULL";
	LOG(Log::ERR, lh ) << msg.str();
	m_gsig->fireSignal( 006, msg.str().c_str() );

	return NULL;
}
}
