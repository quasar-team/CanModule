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

// called by factory
CanLibLoader::CanLibLoader(const std::string& libName)
{
	LogItInstance *logIt = LogItInstance::getInstance();
	logIt->getComponentHandle( CanModule::LogItComponentName, lh );
}

CanLibLoader::~CanLibLoader() {}

CanLibLoader* CanLibLoader::createInstance(const std::string& libName)	{
	CanLibLoader* libPtr = 0;
#ifdef WIN32
	libPtr = new CanLibLoaderWin(libName);
#else
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
CCanAccess* CanLibLoader::openCanBus(string name, string parameters) {
	LOG(Log::DBG, lh ) << __FUNCTION__ << " Creating CCanAccess: name= " << name << " parameters= " << parameters;
	CCanAccess *tcca = createCanAccess();

	if ( !tcca ){
		LOG(Log::ERR, lh ) << __FUNCTION__ << " failed to create CCanAccess name= " << name << " parameters= " << parameters;
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
	 * @param parameters: Different parameters used for the initialisation. For using the default parameters just set this to "Unspecified"
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
		LOG(Log::ERR, lh ) << __FUNCTION__ << " Problem opening canBus for: " << name << " : returning NULL";
		/** we could do 3 things here:
		 * 1. throw an exception and stop everything
		 *   throw std::runtime_error("CanLibLoader::openCanBus: createBus Problem when opening canBus. stop." );
		 * 2. try again looping
		 *   this would go on forever if the bus does not come up
		 * 3. ignore the bus and return 0
		 *   we return NULL in this case and the client has to check the pointer of course
		 *   we decided to do that 6.april.2021 quasar meeting, related to https://its.cern.ch/jira/browse/OPCUA-2248
		 */
		return NULL;
		break;
	}
	default:{
		LOG(Log::WRN, lh ) << __FUNCTION__ << " something else went wrong for: " << name << " : returning NULL";
		return NULL;
		break;
	}
	} // switch

	// never reached
	return NULL;
}
}
