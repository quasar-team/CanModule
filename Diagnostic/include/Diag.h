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
	typedef struct {
		std::string bus;
		std::string lib;
		std::string parameter;
	} CONNECTION_DIAG_t;


	/** beta.v6 "an2" CANGetCounters anagate
	The AnaGate CAN F series models can provide all nine TCP, CAN and error counters
	from the web interface. A detailed description of the counters is available in the
	device status chapter of the AnaGate CAN F series manual.
	*/
	typedef struct {
		unsigned int countTCPRx; // TCP Received counter.
		unsigned int countTCPTx; // TCP Transmitted counter.
		unsigned int countCANRx; // CAN Received counter.
		unsigned int countCANTx; // CAN Transmitted counter.
		unsigned int countCANRxErr; // CAN Bus Receive Error counter.
		unsigned int countCANTxErr; // CAN Bus Transmit Error counter.
		unsigned int countCANRxDisc; // CAN Discarded Rx Full Queue counter.
		unsigned int countCANTxDisc; // CAN Discarded Tx Full Queue counter.
		unsigned int countCANTimeout; // CAN Transmit Timeout counter.
	} PORT_COUNTERS_t;

	typedef struct {
		std::string message;
		long int timestamp;
	} PORT_LOG_ITEM_t;

	typedef struct {
		std::vector<PORT_LOG_ITEM_t> portLog;
	} PORT_LOG_ALL_t;

	typedef struct {
		int temperature; // in deg C
		int uptime;      // in seconds
		unsigned int clientCount; // connected clients for this IP/module/submodule
		std::vector<std::string> clientIps; // decoded into strings, from unsigned int
		std::vector<unsigned int> clientPorts; // array of client ports
		std::vector<long int> clientConnectionTimestamps; // (Unix time) of initial client connection
	} HARDWARE_DIAG_t;


	static void insert_maps( CanLibLoader *lib, CCanAccess *acc, std::string params );
	static void delete_maps( CanLibLoader *lib, CCanAccess *acc );
	static std::vector<Diag::CONNECTION_DIAG_t> get_connections();

	static PORT_LOG_ITEM_t get_lastPortLogItem( CCanAccess *acc );
	static PORT_LOG_ALL_t get_allPortLogItems( CCanAccess *acc );
	static HARDWARE_DIAG_t get_hardwareDiag( CCanAccess *acc );

	static int CanLibLoader_icount;
	static int CanAccess_icount;
	static Log::LogComponentHandle lh;
	static std::map<std::string, CCanAccess *> port_map;
	static std::map<std::string, CanLibLoader *> lib_map;
	static std::map<std::string, std::string> parameter_map;

private:
	Diag();
	~Diag(){};
};

} /* namespace  */

#endif /* CANMODULE_DIAGNOSTIC_DIAG_H_ */
