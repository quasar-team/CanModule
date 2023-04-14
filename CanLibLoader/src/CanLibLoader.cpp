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

/* static */ LogItInstance *CanLibLoader::st_remoteLogIt = NULL;

/* static */ void CanLibLoader::initializeLogging( LogItInstance* remoteInstance ) {
	Log::initializeDllLogging( remoteInstance );
	CanLibLoader::st_remoteLogIt = remoteInstance;
}

/* static */ LogItInstance *CanLibLoader::st_CLgetLogItInstance(){
	return ( CanLibLoader::st_remoteLogIt );
}

// called by factory
CanLibLoader::CanLibLoader(const std::string& libName)
{
	m_gsig = GlobalErrorSignaler::getInstance();

	// must be set by client since we are in a shared lib, using the static method for that. check it here
	if ( CanLibLoader::st_remoteLogIt != NULL ){
		CanLibLoader::st_remoteLogIt->getComponentHandle( CanModule::LogItComponentName, lh );
		LOG(Log::TRC, lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " constructor LogIt remote seems all fine for " << libName;
	} else {
		std::stringstream msg;
		msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " LogIt remote instance is NULL for " << libName;
		std::cout << msg.str() << std::endl;
		m_gsig->fireSignal( 002, msg.str().c_str() );
	}

	/**
	 * lets get clear about the Logit components and their levels at this point
	 */
	std::map<Log::LogComponentHandle, std::string> log_comp_map = Log::getComponentLogsList();
	std::map<Log::LogComponentHandle, std::string>::iterator it;
	LOG(Log::TRC, lh ) << " *** nb of LogIt components= " << log_comp_map.size() << std::endl;

	Log::LOG_LEVEL level;
	for ( it = log_comp_map.begin(); it != log_comp_map.end(); it++ )
	{
		Log::getComponentLogLevel( it->first, level );
		LOG(Log::TRC, lh )<< " *** " << " LogIt component " << it->second << " level= " << level << std::endl;
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
		std::stringstream msg;
		msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " CanBus name to be deleted: " << cInter->getBusName();
		m_gsig->fireSignal( 003, msg.str().c_str() );

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
		m_gsig->fireSignal( 004, msg.str().c_str() );

		throw std::runtime_error("CanLibLoader::openCanBus: createBus Problem when loading lib " + name );
	} else {
		LOG(Log::DBG, lh ) << __FUNCTION__ << " created CCanAccess name= " << name << " parameters= " << parameters;
	}

	/** The LogIt instance of the executable is handled to the DLL at this point, so the instance is shared.
	 * the global logIt instance is kept as private var of the superclass CCanAccess accessible via classical methods
	 * so that the CAN port (~vendor class objects) can use it.
	 */
	tcca->initialiseLogging( CanLibLoader::st_remoteLogIt );

	/** we check if the component "CanModule" is registered and the loglevel is set. If not, sth is wrong, but we can
	 * still register it and set it to TRC: bad luck.
	 */
	Log::LOG_LEVEL loglevel = Log::TRC; // default
	Log::LogComponentHandle handle = Log::getComponentHandle( CanModule::LogItComponentName );
	bool ret = Log::getComponentLogLevel( handle, loglevel );
	if ( ret ) {
		LOG(Log::DBG, lh ) << " got " << CanModule::LogItComponentName << " loglevel= " << loglevel;
	} else {
		LOG(Log::WRN, lh ) << " component " << CanModule::LogItComponentName << " does not exist, register it and set loglevel= " << loglevel;
		CanLibLoader::st_remoteLogIt->registerLoggingComponent( CanModule::LogItComponentName, loglevel );
	}

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
		m_gsig->fireSignal( 005, msg.str().c_str() );

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
		m_gsig->fireSignal( 006, msg.str().c_str() );
		return NULL;
		break;
	}
	} // switch

	// never reached ;-)
	std::stringstream msg;
	msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " something else went seriously wrong for: " << name << " : returning NULL";
	LOG(Log::ERR, lh ) << msg.str();
	m_gsig->fireSignal( 007, msg.str().c_str() );

	return NULL;
}
}
