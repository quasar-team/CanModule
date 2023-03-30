/*
 * UdevAnalyserForPeak.cpp
 *
 *  Created on: Mar 12, 2020
 *      Author: mludwig
 */

#include "UdevAnalyserForPeak.h"
#include "ExecCommand.h"
#include <bits/stdc++.h>

#include <CCanAccess.h>
#include <LogIt.h>


// using namespace CanModule;
namespace udevanalyserforpeak_ns {

/* static, private */ UdevAnalyserForPeak* UdevAnalyserForPeak::m_instance_ptr = 0;
/* static */ Log::LogComponentHandle UdevAnalyserForPeak::m_logItHandleSock = 0;
/* static */ std::vector<UdevAnalyserForPeak::PEAK_device_t> UdevAnalyserForPeak::m_peak_v;

/* private, singleton */ UdevAnalyserForPeak::UdevAnalyserForPeak(){
	LogItInstance* logItInstance = LogItInstance::getInstance();
	if ( !LogItInstance::setInstance(logItInstance))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not set LogIt instance" << std::endl;

	if (!logItInstance->getComponentHandle(CanModule::LogItComponentName, m_logItHandleSock))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not get LogIt component handle for " << CanModule::LogItComponentName << std::endl;

	this->m_createUdevPortMap();
}; // singleton


/**
 * convert a PEAK extended portIdentifier to a socketcan device, i.e.
 *  "sock:can0:device123456" -> "can2"
 *
 *  meaning
 *  -------
 *  input:
 *  PEAK_device_t.localCanPort = 0
 *  PEAK_device_t.deviceID = 123456
 *
 *  returns:
 *  "can2"
 *
 *  This means that the PEAK can bridge with deviceID 123456 is currently the second device
 *  in the socketcan sequence, and that we talk to the first port (locally "can0") of that device. Since the
 *  first device has (obviously) 2 can ports (globally "can0", "can1") we end up with globally "can2"
 *  which is the next in the sequence.
 *     Why do we need this? With peak bridges the socketcan device sequence depends on the USB plugin events
 *  and is therefore not deterministic. Strictly speaking on has no guarantee that "can2" [second bridge, first port]
 *  talks to the same electronics on every day, on monday it might also talk to the firs device.
 *  This stems from the fact that the peak driver does NOT publish the device ID under linux, therefore the udev
 *  system can't pick it up to provide deterministic mapping either.
 *     The fix is
 *     (1) stamp a unique deviceID into the bridge using peak tools under windows. this value is persistent
 *  and will survive a reboot.
 *     (2) under linux, use an extended  peak port identifier : "sock:can0:device123456". CanModule remaps ports under the hood for you
 *     (3) under windows, use the brief port identifier "sock:can2" as before. If you use an extended port identifier the world will explode.
 *     But you can't, anyway, since socketcan is linux only. The peak driver for windows is better: it (seems to) take care of the
 *     device ID properly, BUT DONT ASK ME for that crap...
 */
std::string UdevAnalyserForPeak::peakExtendedIdentifierToSocketCanDevice( std::string extPortId ){
	LOG(Log::TRC, m_logItHandleSock) << "peak extPortId= " << extPortId;

	getInstance(); // make sure we have a map

	// first, sift out the device ID as given
	size_t pos1 = extPortId.find( ":" ) + 1;
	std::string sub1 = extPortId.substr( pos1, std::string::npos );
	//std::cout << __FILE__ << " " << __LINE__ << " sub1= " << sub1 << std::endl;

	size_t pos2 = sub1.find( ":device" ) + 7;
	std::string sub2 = sub1.substr( pos2, std::string::npos );
	//std::cout << __FILE__ << " " << __LINE__ << " sub2= " << sub2 << std::endl;
	int deviceId =  std::stoi( sub2 );

	// second, sift out the local port number as well
	//	size_t pos3 = sub1.find( ":" );
	std::string sub3 = sub1.substr( 0, sub1.find( ":" ) );
	//std::cout << __FILE__ << " " << __LINE__ << " sub3= " << sub3 << std::endl;

	unsigned int localPort = 999999;
	// can have :0 or can0:
	if ( sub3.find("can") == std::string::npos  ){
		localPort =  std::stoi( sub3 );
	} else {
		//std::cout << __FILE__ << " " << __LINE__ << " sub4= " << sub4 << std::endl;
		std::string sub4 = sub3.substr( 3, std::string::npos );
		localPort =  std::stoi( sub4 );
	}

	// look at the map and find out the correct socketcan global port
	LOG(Log::TRC, m_logItHandleSock) << "peak looking for deviceId= " << deviceId
			<< " localPort= " << localPort;

	for ( unsigned int i = 0; i < m_peak_v.size(); i++ ){
		//LOG(Log::TRC, m_logItHandleSock) << "peak comparing " << i
		//		<< " deviceID= " << m_peak_v[ i ].deviceID
		//		<< " localCanPort= " <<  m_peak_v[ i ].localCanPort;

		if (( m_peak_v[ i ].deviceID == deviceId ) && ( m_peak_v[ i ].localCanPort == localPort )){
			LOG(Log::TRC, m_logItHandleSock) << "peak found "
					<< " deviceID= " << deviceId << " localPort= " <<  localPort;
			return( std::string("can") + std::to_string( m_peak_v[ i ].socketNumber ));
		}
	}

	LOG(Log::ERR, m_logItHandleSock) << "peak could not find "
			<< " " << deviceId << " " <<  localPort << " in the udev system" << std::endl;
	return( "" );
}

void UdevAnalyserForPeak::showMap(){
	LOG(Log::INF, m_logItHandleSock) << "here is the map of all PEAK USB-CAN bridges on your system: ";
	for ( unsigned int i = 0; i < m_peak_v.size(); i++ ){
		LOG(Log::INF, m_logItHandleSock)
				<< " deviceID= " << m_peak_v[ i ].deviceID
				<< " systemDeviceIndex= " << m_peak_v[ i ].systemDeviceIndex
				<< " localCanPort= " << m_peak_v[ i ].localCanPort
				<< " socketNumber= " << m_peak_v[ i ].socketNumber
				<< " peakDriverNumber= " << m_peak_v[ i ].peakDriverNumber
				<< " link= " << m_peak_v[ i ].link;
	}
}

/**
 * compares structs peak1 and peak2 and returns TRUE if the peak1->peakDriverNumber is SMALLER
 * than peak2->peakDriverNumber .
 */
/* static */ bool UdevAnalyserForPeak::m_peakStructCompare( PEAK_device_t peak1, PEAK_device_t peak2 ){
	return( peak1.peakDriverNumber < peak2.peakDriverNumber );
}

/**
 * this is where we do the udev call and construct a local-global port map which is
 * system wide: need to scan for all pcan device links
 */
void UdevAnalyserForPeak::m_createUdevPortMap( void ){
	LOG(Log::TRC, m_logItHandleSock) << "peak doing udev calls to discover PEAK socketcan device mapping";

	std::vector<std::string> links1;
	std::vector<std::string> links2;

	// just the real devices, not the symlinks
	std::string cmd0 = "ls -l /dev/pcanusb* | grep -v \" -> \" | awk '{print $10}' ";
	execcommand_ns::ExecCommand exec0( cmd0 );
	const execcommand_ns::ExecCommand::CmdResults results0 = exec0.getResults();
	LOG(Log::TRC, m_logItHandleSock) << "peak exec0= " << exec0;

	// get the symlinks for each device and the first port
	for ( unsigned int i = 0; i < results0.size(); i++ ){
		std::string cmd1 = std::string("/sbin/udevadm info -q symlink ") + results0[ i ] + std::string(" | grep \"devid=\"");
		LOG(Log::TRC, m_logItHandleSock) << "peak cmd1= " << cmd1;
		execcommand_ns::ExecCommand exec1( cmd1 );
		execcommand_ns::ExecCommand::CmdResults results1 = exec1.getResults();
		for ( unsigned k = 0; k < results1.size(); k++ ){
			links1.push_back( results1[ k ] );
			LOG(Log::TRC, m_logItHandleSock) << "peak results1[ " << k << "]=" << results1[ k ];
		}
	}
	// get the links of the other ports
	for ( unsigned int i = 0; i < results0.size(); i++ ){
		std::string cmd2 = std::string("/sbin/udevadm info -q symlink ") + results0[ i ] + std::string(" | grep -v \"devid=\"");
		LOG(Log::TRC, m_logItHandleSock) << "peak cmd2= " << cmd2;
		execcommand_ns::ExecCommand exec2( cmd2 );
		execcommand_ns::ExecCommand::CmdResults results2 = exec2.getResults();
		for ( unsigned k = 0; k < results2.size(); k++ ){
			links2.push_back( results2[ k ] );
			LOG(Log::TRC, m_logItHandleSock) << "peak results2[ " << k << "]="  << results2[ k ];
		}
	}

	/** now, build the map from results1:
	 * pcan-usb_pro_fd/0/can0 pcan-usb_pro_fd/devid=9054 pcan32 pcanusbpfd32
	 * pcan-usb_pro_fd/1/can0 pcan-usb_pro_fd/devid=8910 pcan34 pcanusbpfd34
	 *
	 * and result2:
	 * pcan-usb_pro_fd/0/can1 pcan33 pcanusbpfd33
	 * pcan-usb_pro_fd/1/can1 pcan35 pcanusbpfd35
	 */

	for ( unsigned int i = 0; i < links1.size(); i++ ){
		PEAK_device_t peak;
		peak.deviceID = m_peakDeviceId( links1[ i ] );
		peak.systemDeviceIndex = m_peakSystemDeviceIndex( links1[ i ] );
		peak.localCanPort = m_peakLocalCanPort( links1[ i ] );
		peak.peakDriverNumber = m_peakDriverNumber( links1[ i ] );
		peak.link = links1[ i ];
		m_peak_v.push_back( peak );
	}
	for ( unsigned int i = 0; i < links2.size(); i++ ){
		PEAK_device_t peak;
		peak.systemDeviceIndex = m_peakSystemDeviceIndex( links2[ i ] );
		peak.deviceID = m_peakDeviceIdFromSystemDeviceIndex( peak.systemDeviceIndex );
		peak.localCanPort = m_peakLocalCanPort( links2[ i ] );
		peak.peakDriverNumber = m_peakDriverNumber( links2[ i ] );
		peak.link = links2[ i ];
		m_peak_v.push_back( peak );
	}

	/**
	 * now we have to sort this with key=peakDriverNumber, since that is what socketcan does when assigning
	 * the socketcan devices
	 */
	sort( m_peak_v.begin(), m_peak_v.end(), UdevAnalyserForPeak::m_peakStructCompare );
	for ( unsigned i = 0; i < m_peak_v.size(); i++ ){
		m_peak_v[ i ].socketNumber = i;
	}
	showMap();
}

/**
 * get the deviceID from the system driver number
 */
unsigned int UdevAnalyserForPeak::m_peakDeviceIdFromSystemDeviceIndex( unsigned int sd ){
	for ( std::vector<PEAK_device_t>::iterator it = m_peak_v.begin(); it <= m_peak_v.end(); it++ ){
		if ( it->systemDeviceIndex == sd ) return( it->deviceID );
	}
	return(-1); // not found
}

/**
 * get the local socket nb of the devices: "32"
 *
 * cc7:
 * pcan-usb_pro_fd/0/can0 pcan-usb_pro_fd/devid=9054 pcan32 pcanusbpfd32
 *
 * cal9 reports differently:
 * pcan32 pcanusbpfd32 pcan-usb_pro_fd/0/can0 pcan-usb_pro_fd/devid=9054
 * pcan-usb_pro_fd/devid=9054 pcan32 pcan-usb_pro_fd/0/can0 pcanusbpfd32
 *
 */
unsigned int UdevAnalyserForPeak::m_peakDriverNumber( std::string s ){
	std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " extracting peak driver number from " << s << std::endl;


	size_t pos1 = s.find( "devid=" ) + 6;
	std::string sub1 = s.substr( pos1, std::string::npos );
	std::cout << __FILE__ << " " << __LINE__ << " extended device id devid= " << sub1 << std::endl;




	size_t pos2 = s.find( "pcan" );
	//std::string sub2 = sub1.substr( pos2, std::string::npos );
	std::string sub2 = sub1.substr( pos2, std::string::npos  ); // two digit driver numbers
	std::cout << __FILE__ << " " << __LINE__ << " driver number sub2= " << sub2 << std::endl;
	std::string sub3 = sub2.substr( 0, sub2.find(" ") );
	std::cout << __FILE__ << " " << __LINE__ << " DEBUG sub3= " << sub3 << " stoi(sub3)= " << std::stoi( sub3 ) << std::endl;
	int ii = std::stoi( sub3, 0, 10 );
	std::cout << __FILE__ << " " << __LINE__ << " DEBUG i4= " << ii << std::endl;

	return( (unsigned int) ii );

}

/**
 * get the local can port of the devices: the "0" of "can0"
 * pcan-usb_pro_fd/0/can0 pcan-usb_pro_fd/devid=9054 pcan32 pcanusbpfd32
 */
unsigned int UdevAnalyserForPeak::m_peakLocalCanPort( std::string s ){
	size_t pos1 = s.find( "/" ) + 1;
	std::string sub1 = s.substr( pos1, std::string::npos );
	size_t pos2 = sub1.find( "/" ) + 1;
	std::string sub2 = sub1.substr( pos2, sub1.find(" ") - 2 );
	std::string sub3 = sub2.substr( 3, std::string::npos );
	std::cout << __FILE__ << " " << __LINE__ << " sub3= " << sub3 << std::endl;
	int ii = std::stoi( sub3, 0, 10 );
	std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " ii= " << ii << std::endl;
	return( (unsigned int) ii );
}

/**
 * get the devids of the devices "9054"
 * pcan-usb_pro_fd/0/can0 pcan-usb_pro_fd/devid=9054 pcan32 pcanusbpfd32
 */
unsigned int UdevAnalyserForPeak::m_peakDeviceId( std::string s ){
	const std::string devid = "devid=";
	size_t pos1 = s.find( devid ) + std::string( devid ).length();
	std::string sub1 = s.substr( pos1, std::string::npos );
	std::string sub2 = sub1.substr( 0, sub1.find(" ") );
	std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " sub2= " << sub2 << std::endl;
	int ii = std::stoi( sub2, 0, 10 );
	std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " ii= " << ii << std::endl;
	return( (unsigned int) ii );
}

/**
 * get the global device index "0"
 * pcan-usb_pro_fd/0/can0 pcan-usb_pro_fd/devid=9054 pcan32 pcanusbpfd32
 */
unsigned int UdevAnalyserForPeak::m_peakSystemDeviceIndex( std::string s ){
	size_t pos1 = s.find( "/" ) + 1;
	std::string sub1 = s.substr( pos1, std::string::npos );
	std::string sub2 = sub1.substr( 0, sub1.find("/") );
	std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " sub2= " << sub2 << std::endl;
	int ii = std::stoi( sub2, 0, 10 );
	std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " ii= " << ii << std::endl;
	return( (unsigned int) ii );
}


} // namespace udevanalyserforpeak_ns


