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
#include <mutex>

#include <CCanAccess.h>
#include <CanLibLoader.h>

namespace CanModule {


/**
 * diagnostic class to monitor what is going on inside CanModule, used as static api only
 * - keeps track of library instances and pointers
 * - tracks opened can buses for each lib instance
 */
class Diag {
public:

	const int portLog_max = 5000;

	typedef struct {
		std::string bus;
		std::string lib;
		std::string parameter;
	} CONNECTION_DIAG_t;

	static void insert_maps( CanLibLoader *lib, CCanAccess *acc, std::string params );
	static void delete_maps( CanLibLoader *lib, CCanAccess *acc );
	static std::vector<Diag::CONNECTION_DIAG_t> get_connections();

	/**
	 * these guys actually talk to the hardware to get the info. They check before if the hardware has
	 * actually implemented those diags. If not, just returns empty data. Well, in case of trouble, you do NOT
	 * want any fancy time-synced and mutex protected buffering going on behind the scenes for this diag stuff,
	 * even if this would be better for the performance. So: this will actually use HW resources, use sparingly.
	 * this is also why you get rather unmodern typedef struct data as well, returned as values.
	*/
	static OptionalVector<CanModule::PORT_LOG_ITEM_t> get_portLogItems( CCanAccess *acc, int n );
	static std::optional<CanModule::HARDWARE_DIAG_t> get_hardwareDiag( CCanAccess *acc );
	static std::optional<CanModule::PORT_COUNTERS_t> get_portCounters( CCanAccess *acc );

	static int CanLibLoader_icount;
	static int CanAccess_icount;
	static Log::LogComponentHandle lh;
	static std::map<std::string, CCanAccess *> port_map;
	static std::map<std::string, CanLibLoader *> lib_map;
	static std::map<std::string, std::string> parameter_map;

	SHARED_LIB_EXPORT_DEFN static CanModule::PORT_LOG_ITEM_t createEmptyItem();
	SHARED_LIB_EXPORT_DEFN static CanModule::HARDWARE_DIAG_t createEmptyDiag();
	SHARED_LIB_EXPORT_DEFN static CanModule::PORT_COUNTERS_t createEmptyCounters();

private:
	Diag();
	~Diag(){};
};

} /* namespace  */

#endif /* CANMODULE_DIAGNOSTIC_DIAG_H_ */
