/*
 * Diag.cpp
 *
 *  Created on: Aug 25, 2020
 *      Author: mludwig
 */

#include "Diag.h"

namespace CanModule {

/* static */ int Diag::CanLibLoader_icount = 0;
/* static */ int Diag::CanAccess_icount = 0;


// /* static */ Log::LogComponentHandle Diag::lh = 0;
/* static */ std::map<std::string, CCanAccess *> Diag::port_map;
/* static */ std::map<std::string, CanLibLoader *> Diag::lib_map;
/* static */ std::map<std::string, std::string> Diag::parameter_map;

std::mutex mtx;

// should be a forced singleton
Diag::Diag() {};

/**
 * find out id the implementation has advanced port diags, from the bus name. Currently only
 * anagate2 has this kind of diags. Other implementations have some of it as well, notably the statistics.
 * Nevertheless, this is from the hardware.
 *
 * can't force that as private static virtual = 0 method: windows actually compiles this differently. So,
 * implement it here explicitly scanning for the implementation name, here: only "an2" has it for the moment.
 */
/* private */ bool Diag::m_implemenationHasDiags( CCanAccess *acc ){
	std::string busName = acc->getBusName();
	if ( busName.find("an2") != std::string::npos ) return true;
	return false;
}

/* private */ Diag::PORT_LOG_ITEM_t Diag::m_createEmptyItem(){
	Diag::PORT_LOG_ITEM_t item;
	item.message = "no port log message";
	item.timestamp = 0;
	return item;
}

/* private */ Diag::HARDWARE_DIAG_t Diag::m_createEmptyDiag(){
	Diag::HARDWARE_DIAG_t d;
	d.temperature = 0;
	d.uptime = 0;
	d.clientCount = 0;
	d.clientIps.push_back("000.000.000.000");
	d.clientPorts.push_back(0);
	d.clientConnectionTimestamps.push_back( 0 );
	return d;
}

void Diag::delete_maps(CanLibLoader *lib, CCanAccess *acc ){

	std::string c0 = lib->getLibName() + "_" + std::to_string( CanLibLoader_icount );
	std::string c1 = acc->getBusName() + "_" + std::to_string( CanAccess_icount );
	std::string key = c0 + ":" + c1;

	mtx.lock();
	{
		std::map<std::string, CCanAccess *>::iterator it = port_map.find( key );
		if ( it != port_map.end() )
			port_map.erase( key );
	}

	{
		std::map<std::string, CanLibLoader *>::iterator it = lib_map.find( key );
		if ( it != lib_map.end() )
			lib_map.erase( key );
	}

	{
		std::map<std::string, std::string>::iterator it = parameter_map.find( key );
		if ( it != parameter_map.end() )
			parameter_map.erase( key );
	}
	mtx.unlock();
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
	std::string key = c0 + "::" + c1;

	mtx.lock();
	if ( lib_map.find( key ) != lib_map.end()) {
		std::cout  << " key= " << key << " exists already, skip lib insert" << std::endl;
		//LOG(Log::INF, lh )<< " key= " << key << " exists already, skip lib insert";
	} else	{
		std::pair<std::string, CanLibLoader *> pa0 = std::pair<std::string, CanLibLoader *>( key, lib );
		lib_map.insert( pa0 );
	}
	if ( port_map.find( key ) != port_map.end()) {
		//LOG(Log::INF, lh )<< " key= " << key << " exists already, skip port insert";
		std::cout  << " key= " << key << " exists already, skip port insert" << std::endl;
	} else	{
		std::pair<std::string, CCanAccess *> pa1 = std::pair<std::string, CCanAccess *>( key, acc );
		port_map.insert( pa1 );
	}
	if ( parameter_map.find( key ) != parameter_map.end()) {
		// LOG(Log::INF, lh )<< " key= " << key << " exists already, skip parameter insert";
		std::cout << " key= " << key << " exists already, skip parameter insert" << std::endl;
	} else	{
		std::pair<std::string, std::string> pa2 = std::pair<std::string, std::string>( key, params );
		parameter_map.insert( pa2 );
	}
	mtx.unlock();
}

/**
 * gives back a vector filled with connection details, mostly timestamp related
 * the maps are only used as rvalues in a reentrant method: fine without mutex
 */
std::vector<Diag::CONNECTION_DIAG_t> Diag::get_connections(){
	std::vector<Diag::CONNECTION_DIAG_t> vreturn;
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


/**
 * read from hw the last log entries, 10 or less (or none)
 */
std::vector<Diag::PORT_LOG_ITEM_t> Diag::get_last10PortLogItems( CCanAccess *acc ){
	std::vector<Diag::PORT_LOG_ITEM_t> items;
	int count = 10;
	if ( Diag::m_implemenationHasDiags( acc ) ){
		// it is an anagate2: go out and fill the data
		/**	AnaInt32 CANGetLog(AnaInt32 hHandle, AnaUInt32 * nLogID, AnaUInt32
		 * pnCurrentID, AnaUInt32 * pnLogCount, AnaInt64 * pnLogDate, char
		pcBuffer[]);
		 */


		return items;
	}
	return items;
};

std::vector<Diag::PORT_LOG_ITEM_t> Diag::get_allPortLogItems( CCanAccess *acc ){
	std::vector<Diag::PORT_LOG_ITEM_t> items;
	// int count = 10;
	if ( Diag::m_implemenationHasDiags( acc ) ){
		// it is an anagate2: go out and fill the data
		/**	AnaInt32 CANGetLog(AnaInt32 hHandle, AnaUInt32 * nLogID, AnaUInt32
		 * pnCurrentID, AnaUInt32 * pnLogCount, AnaInt64 * pnLogDate, char
		pcBuffer[]);
		 */


		return items;
	}
	return items;

};

Diag::HARDWARE_DIAG_t Diag::get_hardwareDiag( CCanAccess *acc ){
	Diag::HARDWARE_DIAG_t diag;
	if ( Diag::m_implemenationHasDiags( acc ) ){
		/**
		 * 	AnaInt32 CANGetDiagData(AnaInt32 hHandle, AnaInt32 * pnTemperature,
		 * 			AnaInt32 * pnUptime);int temperature; // in deg C
		 *
		 * 	AnaInt32 CANGetClientList(AnaInt32 hHandle, AnaUInt32 * pnClientCount,
		 * 				AnaUInt32 pnIPaddress[], AnaUInt32 pnPort[], AnaInt64 pnConnectDate[]);
		 */
		//int uptime;      // in seconds
		//unsigned int clientCount; // connected clients for this IP/module/submodule
		//std::vector<std::string> clientIps; // decoded into strings, from unsigned int
		//std::vector<unsigned int> clientPorts; // array of client ports
		//std::vector<long int> clientConnectionTimestamps; // (Unix time) of initial client connection
	} else {
		diag = m_createEmptyDiag();
	}
	return diag;
};


} /* namespace */
