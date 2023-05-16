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

	/**
	 * a (hardware) logging item generally has a message and a timestamp. Keep it simple.
	 *
	 * a log entry history of one port has many log items. anagate2 hw limits this to 100 but there is no reason to limit the vector size here
	 * we can in fact keep a looooong history. we will just limit the max history to a reasonable value to avoid a coded mem hole.
	 * Lets say portLog_max = 5000.
	 * we drop any buffer overflow.
	 *
	 */
	typedef struct {
		std::string message;
		long int timestamp;
	} PORT_LOG_ITEM_t;

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

	/**
	 * these guys actually talk to the hardware to get the info. They check before if the hardware has
	 * actually implemented those diags. If not, just returns empty data. Well, in case of trouble, you do NOT
	 * want any fancy time-synced and mutex protected buffering going on behind the scenes for this diag stuff,
	 * even if this would be better for the performance. So: this will actually use HW resources, use sparingly.
	 * this is also why you get rather unmodern typedef struct data as well, returned as values.
	*/
	static std::vector<PORT_LOG_ITEM_t> get_last10PortLogItems( CCanAccess *acc );
	static std::vector<PORT_LOG_ITEM_t> get_allPortLogItems( CCanAccess *acc );
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

	static bool m_implemenationHasDiags( CCanAccess *acc );
	static PORT_LOG_ITEM_t m_createEmptyItem();
	static Diag::HARDWARE_DIAG_t m_createEmptyDiag();

};

} /* namespace  */

#endif /* CANMODULE_DIAGNOSTIC_DIAG_H_ */
