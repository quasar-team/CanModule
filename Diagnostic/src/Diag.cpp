/*
 * Diag.cpp
 *
 *  Created on: Aug 25, 2020
 *      Author: mludwig
 */

#include "Diag.h"

namespace CanModule {

Diag::Diag():
	CanLibLoader_icount(0),
	CanAccess_icount(0)
{
	LogItInstance *logIt = LogItInstance::getInstance();
	logIt->getComponentHandle( CanModule::LogItComponentName, lh );
};

void Diag::delete_maps(CanLibLoader *lib, CCanAccess *acc ){
	std::string c0 = lib->getLibName() + "_" + std::to_string( CanLibLoader_icount );
	std::string c1 = acc->getBusName() + "_" + std::to_string( CanAccess_icount );
	std::string key = c0 + ":" + c1;

	port_map.erase( key );
	lib_map.erase( key );
	parameter_map.erase( key );
}

/**
 * insert the two objects into the tracing maps and define a common key
 * the key looks like this:  libname_CanLibLoader_icount:busname_CanAccess_icount
 * i.e.
 * an_1:can2_0  [ anagate lib instance 1, bus can2 instance 0 ]
 * sock_0:can2_4  [ sock instance 0, bus can2 instance 4 ]
 */
void Diag::insert_maps( CanLibLoader *lib, CCanAccess *acc, std::string params ){
	std::string c0 = lib->getLibName() + "_" + std::to_string( CanLibLoader_icount );
	std::string c1 = acc->getBusName() + "_" + std::to_string( CanAccess_icount );
	std::string key = c0 + ":" + c1;

	LOG(Log::TRC, lh ) << " c0= " << c0 << " c1= " << c1 << " key= " << key;

	if ( lib_map.find( key ) != lib_map.end()) {
		LOG(Log::INF, lh )<< " key= " << key << " exists already, skip lib insert";
	} else	{
		std::pair<std::string, CanLibLoader *> pa0 = std::pair<std::string, CanLibLoader *>( key, lib );
		lib_map.insert( pa0 );
	}
	if ( port_map.find( key ) != port_map.end()) {
		LOG(Log::INF, lh )<< " key= " << key << " exists already, skip port insert";
	} else	{
		std::pair<std::string, CCanAccess *> pa1 = std::pair<std::string, CCanAccess *>( key, acc );
		port_map.insert( pa1 );
	}
	if ( parameter_map.find( key ) != parameter_map.end()) {
		LOG(Log::INF, lh )<< " key= " << key << " exists already, skip parameter insert";
	} else	{
		std::pair<std::string, std::string> pa2 = std::pair<std::string, std::string>( key, params );
		parameter_map.insert( pa2 );
	}
}

vector<Diag::CONNECTION_DIAG_t> Diag::get_connections(){
	vector<Diag::CONNECTION_DIAG_t> vreturn;
	for (std::map<std::string, CCanAccess *>::iterator it=port_map.begin(); it!=port_map.end(); ++it){
		Diag::CONNECTION_DIAG_t c;
		std::string key = it->first;
		c.bus = it->second->getBusName();
		c.lib = lib_map.find( key )->second->getLibName();
		c.parameter = parameter_map.find( key )->second;
		vreturn.push_back( c );
	}
	return( vreturn );
};


} /* namespace */
