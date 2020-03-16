/*
 * UdevAnalyserForPeak.h
 *
 *  Created on: Mar 12, 2020
 *      Author: mludwig
 */

#ifndef CANMODULE_CANINTERFACEIMPLEMENTATIONS_SOCKCAN_UDEVANALYSERFORPEAK_H_
#define CANMODULE_CANINTERFACEIMPLEMENTATIONS_SOCKCAN_UDEVANALYSERFORPEAK_H_


#include <string>
#include <vector>

#include <LogIt.h>
// #include <CCanAccess.h>
// #include "CCanAccess.h"

// using namespace CanModule;

namespace udevanalyserforpeak_ns {

//class UdevAnalyserForPeak : public CCanAccess
class UdevAnalyserForPeak {
public:
	typedef struct {
		std::string link;
		unsigned int localCanPort; // local: i.e. can0, can1
		unsigned int systemDeviceIndex; // global: i.e. pcan-usb_pro_fd/0
		unsigned int peakDriverNumber; // global: i.e. pcan33
		int deviceID; // global, must be unique (serial#), i.e. devid=9054, -1 if not found for secondary ports
		unsigned int socketNumber;
	} PEAK_device_t;

	// factory providing one singleton
	static UdevAnalyserForPeak* getInstance() {
		if ( !m_instance_ptr )
			m_instance_ptr = new UdevAnalyserForPeak;
		return m_instance_ptr;
	}

	static std::string portIdentifierToSocketCanDevice( std::string portId );
	static void showMap();

private:
	// singleton, allocated at compile time, static.
	static UdevAnalyserForPeak *m_instance_ptr;
	UdevAnalyserForPeak(); // private constructor singleton
	UdevAnalyserForPeak(const UdevAnalyserForPeak&); // no copy
	virtual ~UdevAnalyserForPeak(){};

	static std::vector<PEAK_device_t> m_peak_v;
	static Log::LogComponentHandle m_logItHandleSock;

	void m_createUdevPortMap(); // udev calls in constructor

	unsigned int m_peakDeviceId( std::string s );
	unsigned int m_peakSystemDeviceIndex( std::string s );
	unsigned int m_peakLocalCanPort( std::string s );
	unsigned int m_peakDriverNumber( std::string s );
	unsigned int m_peakDeviceIdFromSystemDeviceIndex( unsigned int sd );

	static bool m_peakStructCompare( PEAK_device_t peak1, PEAK_device_t peak2 ); // sort method

};

} /* namespace udevanalyserforpeak_ns */

#endif /* CANMODULE_CANINTERFACEIMPLEMENTATIONS_SOCKCAN_UDEVANALYSERFORPEAK_H_ */
