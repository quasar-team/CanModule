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

namespace udevanalyserforpeak_ns {

class UdevAnalyserForPeak {
public:
	UdevAnalyserForPeak(){};
	virtual ~UdevAnalyserForPeak(){};
    static int portMap( void );

private:
    static unsigned int _peakDeviceId( std::string s );
    static unsigned int _peakSystemDeviceIndex( std::string s );
    static std::string _peakLocalCanPort( std::string s );
    static unsigned int _peakDriverNumber( std::string s );

};

} /* namespace udevanalyserforpeak_ns */

#endif /* CANMODULE_CANINTERFACEIMPLEMENTATIONS_SOCKCAN_UDEVANALYSERFORPEAK_H_ */
