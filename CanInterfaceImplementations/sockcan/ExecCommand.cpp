/*
 * ExecCommand.cpp
 *
 *  Created on: Mar 9, 2020
 *      Author: mludwig
 */

#include "ExecCommand.h"
#include <stdlib.h>
#include <vector>
#include <string>

namespace execcommand_ns {
/**
 * this is where we do the udev call and construct a locl-global port map which is
 * system wide: need to scan for all pcan device links
 */
/* static */ int ExecCommand::portMap( void ){

	std::vector<std::string> links1;
	std::vector<std::string> links2;

	// just the real devices, not the symlinks
	std::string cmd0 = "ls -l /dev/pcanusb* | grep -v \" -> \" | awk '{print $10}' ";
	execcommand_ns::ExecCommand exec0( cmd0 );
	const execcommand_ns::ExecCommand::CmdResults results0 = exec0.getResults();
	std::cout << exec0; // << std::endl;

	// get the symlinks for each device and the first port
	for ( unsigned int i = 0; i < results0.size(); i++ ){
		std::string cmd1 = std::string("/sbin/udevadm info -q symlink ") + results0[ i ] + std::string(" | grep \"devid=\"");
		execcommand_ns::ExecCommand exec1( cmd1 );
		execcommand_ns::ExecCommand::CmdResults results1 = exec1.getResults();
		std::cout << exec1; // << std::endl;
		for ( unsigned k = 0; k < results1.size(); k++ ){
			links1.push_back( results1[ k ] );
		}
	}
	// get the links of the other ports
	for ( unsigned int i = 0; i < results0.size(); i++ ){
		std::string cmd2 = std::string("/sbin/udevadm info -q symlink ") + results0[ i ] + std::string(" | grep -v \"devid=\"");
		execcommand_ns::ExecCommand exec2( cmd2 );
		execcommand_ns::ExecCommand::CmdResults results2 = exec2.getResults();
		std::cout << exec2; // << std::endl;
		for ( unsigned k = 0; k < results2.size(); k++ ){
			links2.push_back( results2[ k ] );
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
	typedef struct {
		std::string localCanPort; // local: i.e. can0, can1
		unsigned int systemDeviceIndex; // global: i.e. pcan-usb_pro_fd/0
		unsigned int peakDriverNumber; // global: i.e. pcan33
		int deviceID; // global, must be unique (serial#), i.e. devid=9054, -1 if not found for secondary ports
		unsigned int socketNumber;
	} PEAK_device_t;
	std::vector<PEAK_device_t> peak_v;

	for ( unsigned int i = 0; i < links1.size(); i++ ){
		PEAK_device_t peak;
//		std::cout << __FILE__ << " " << __LINE__ << " i= " << i << " links1= " << links1[ i ] << std::endl;
		peak.deviceID = _peakDeviceId( links1[ i ] );
//		std::cout << __FILE__ << " " << __LINE__ << " peak.deviceID= " << peak.deviceID << std::endl;

		peak.systemDeviceIndex = _peakSystemDeviceIndex( links1[ i ] );
//		std::cout << __FILE__ << " " << __LINE__ << " peak.systemDeviceIndex= " << peak.systemDeviceIndex << std::endl;

		peak.localCanPort = _peakLocalCanPort( links1[ i ] );
//		std::cout << __FILE__ << " " << __LINE__ << " peak.localCanPort= " << peak.localCanPort << std::endl;

		peak.peakDriverNumber = _peakDriverNumber( links1[ i ] );
//		std::cout << __FILE__ << " " << __LINE__ << " peak.socketNumber= " << peak.socketNumber << std::endl;
		peak_v.push_back( peak );
	}
	for ( unsigned int i = 0; i < links2.size(); i++ ){
		PEAK_device_t peak;
//		std::cout << __FILE__ << " " << __LINE__ << " i= " << i << " links2= " << links2[ i ] << std::endl;
		peak.deviceID = -1;
//		std::cout << __FILE__ << " " << __LINE__ << " peak.deviceID= " << peak.deviceID << std::endl;

		peak.systemDeviceIndex = _peakSystemDeviceIndex( links2[ i ] );
//		std::cout << __FILE__ << " " << __LINE__ << " peak.systemDeviceIndex= " << peak.systemDeviceIndex << std::endl;

		peak.localCanPort = _peakLocalCanPort( links2[ i ] );
//		std::cout << __FILE__ << " " << __LINE__ << " peak.localCanPort= " << peak.localCanPort << std::endl;

		peak.peakDriverNumber = _peakDriverNumber( links2[ i ] );
//		std::cout << __FILE__ << " " << __LINE__ << " peak.socketNumber= " << peak.socketNumber << std::endl;
		peak_v.push_back( peak );
	}

	for ( unsigned i = 0; i < peak_v.size(); i++ ){
		std::cout << __FILE__ << " " << __LINE__ << std::endl
				<< " peak.deviceID= " << peak_v[ i ].deviceID
				<< " peak.systemDeviceIndex= " << peak_v[ i ].systemDeviceIndex
				<< " peak.localCanPort= " << peak_v[ i ].localCanPort
				<< " peak.peakDriverNumber= " << peak_v[ i ].peakDriverNumber
				<< std::endl;
	}

	/**
	 * now we have to sort this with key=peakDriverNumber, since that is what socketcan does when assigning
	 * the socketcan devices
	 */




	return (-1);
}

/**
 * get the local socket nb of the devices: "32"
 * pcan-usb_pro_fd/0/can0 pcan-usb_pro_fd/devid=9054 pcan32 pcanusbpfd32
 */
unsigned int ExecCommand::_peakDriverNumber( std::string s ){
	size_t pos1 = s.find( "devid=" ) + 6;
	std::string sub1 = s.substr( pos1, std::string::npos );
//	std::cout << __FILE__ << " " << __LINE__ << " sub1= " << sub1 << std::endl;
	size_t pos2 = sub1.find( " pcan" ) + 5;
	std::string sub2 = sub1.substr( pos2, std::string::npos );
//	std::cout << __FILE__ << " " << __LINE__ << " sub2= " << sub2 << std::endl;
	std::string sub3 = sub2.substr( 0, sub2.find(" ") );
//	std::cout << __FILE__ << " " << __LINE__ << " sub3= " << sub3 << std::endl;
	return( std::stoi( sub3 ));
}
/**
 * get the local can port of the devices: "can0"
 * pcan-usb_pro_fd/0/can0 pcan-usb_pro_fd/devid=9054 pcan32 pcanusbpfd32
 */
std::string ExecCommand::_peakLocalCanPort( std::string s ){
	size_t pos1 = s.find( "/" ) + 1;
	std::string sub1 = s.substr( pos1, std::string::npos );
	size_t pos2 = sub1.find( "/" ) + 1;
	std::string sub2 = sub1.substr( pos2, sub1.find(" ") - 2 );
	return( sub2 );
}

/**
 * get the devids of the devices "9054"
 * pcan-usb_pro_fd/0/can0 pcan-usb_pro_fd/devid=9054 pcan32 pcanusbpfd32
 */
unsigned int ExecCommand::_peakDeviceId( std::string s ){
	const std::string devid = "devid=";
	size_t pos1 = s.find( devid ) + std::string( devid ).length();
	std::string sub1 = s.substr( pos1, std::string::npos );
	std::string sub2 = sub1.substr( 0, sub1.find(" ") );
	return( std::stoi( sub2 ));
}

/**
 * get the global device index "0"
 * pcan-usb_pro_fd/0/can0 pcan-usb_pro_fd/devid=9054 pcan32 pcanusbpfd32
 */
unsigned int ExecCommand::_peakSystemDeviceIndex( std::string s ){
	size_t pos1 = s.find( "/" ) + 1;
	std::string sub1 = s.substr( pos1, std::string::npos );
	std::string sub2 = sub1.substr( 0, sub1.find("/") );
	return( std::stoi( sub2 ));
}
//bool _peakStructCompare( PEAK_device_t &peak1, PEAK_device_t &peak2 ){
//
//}


} /* namespace execcommand_ns */
