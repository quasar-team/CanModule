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
#if 0
	// singleton
	static Diag& instance(){
		static Diag INSTANCE;
		return INSTANCE;
	}


	virtual ~Diag(){};
#endif
	typedef struct {
		std::string bus;
		std::string lib;
		std::string parameter;
	} CONNECTION_DIAG_t;

	static void insert_maps( CanLibLoader *lib, CCanAccess *acc, std::string params );
	static void delete_maps( CanLibLoader *lib, CCanAccess *acc );
	static vector<Diag::CONNECTION_DIAG_t> get_connections();

// private:
	Diag();
	static int CanLibLoader_icount;
	static int CanAccess_icount;
	static Log::LogComponentHandle lh;

	/* we need to track all objects using a global map
	 * the common map key for all of these maps is a composed string:
	 * libname_CanLibLoader_icount:name_CanAccess_icount
	 */
	static std::map<std::string, CCanAccess *> port_map;
	static std::map<std::string, CanLibLoader *> lib_map;
	static std::map<std::string, std::string> parameter_map;
};

} /* namespace  */

#endif /* CANMODULE_DIAGNOSTIC_DIAG_H_ */
