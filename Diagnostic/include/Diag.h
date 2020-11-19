/*
 * Diag.h
 *
 *  Created on: Aug 25, 2020
 *      Author: mludwig
 */

#ifndef CANMODULE_DIAGNOSTIC_DIAG_H_
#define CANMODULE_DIAGNOSTIC_DIAG_H_

#ifdef _WIN32
#else
#include <sys/time.h>
#endif

#include <map>
#include <string>
#include <iostream>

#include <LogIt.h>

#include <CCanAccess.h>
#include <CanLibLoader.h>

namespace CanModule {


/**
 * diagnostic class to monitor what is going on inside CanModule, used as a singleton.
 * - keeps track of library instances and pointers
 * - tracks opened can buses for each lib instance
 */
class Diag {
public:
	// singleton
	static Diag& instance(){
		static Diag INSTANCE;
		return INSTANCE;
	}

	virtual ~Diag(){};

	void insert_maps( CanLibLoader *lib, CCanAccess *acc, std::string params );
	void delete_maps( CanLibLoader *lib, CCanAccess *acc );

	typedef struct {
		std::string bus;
		std::string lib;
		std::string parameter;
	} CONNECTION_DIAG_t;


#if 0
	std::map<std::string, CCanAccess *> get_port_map(){ return port_map; }
	std::map<std::string, CanLibLoader *> get_lib_map(){ return lib_map; }
	std::map<std::string, std::string> get_parameter_map(){ return parameter_map; }
#endif
	vector<Diag::CONNECTION_DIAG_t> get_connections(){
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

private:
	Diag();
	int CanLibLoader_icount;
	int CanAccess_icount;
	Log::LogComponentHandle lh;

	/* we need to track all objects using a global map
	 * the common map key for all of these maps is a composed string:
	 * libname_CanLibLoader_icount:name_CanAccess_icount
	 */
	std::map<std::string, CCanAccess *> port_map;
	std::map<std::string, CanLibLoader *> lib_map;
	std::map<std::string, std::string> parameter_map;
};

} /* namespace  */

#endif /* CANMODULE_DIAGNOSTIC_DIAG_H_ */
