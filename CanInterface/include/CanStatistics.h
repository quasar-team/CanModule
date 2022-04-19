/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanStatistics.h
 *
 *  Created on: Mar 17, 2015
 *      Author: Piotr Nikiel <piotr@nikiel.info>, quasar team
 *
 *  This file is part of Quasar.
 *
 *  Quasar is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public Licence as published by
 *  the Free Software Foundation, either version 3 of the Licence.
 *
 *  Quasar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public Licence for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Quasar.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CANINTERFACE_SOCKCAN_CANSTATISTICS_H_
#define CANINTERFACE_SOCKCAN_CANSTATISTICS_H_


#ifdef _WIN32
#	include <atomic>
#	include <time.h>
#	ifndef timeval
#		include "Winsock2.h"
#	endif
#else
#	define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#	if GCC_VERSION > 40800
#		include <atomic>
#	else // GCC_VERSION
#		include <stdatomic.h>
#	endif // GCC_VERSION
#	include <sys/time.h>
#endif
#include "ExportDefinition.h"

#include <chrono>
using namespace std;
using namespace std::chrono;

namespace CanModule
{
class CanStatistics
{

public:

	SHARED_LIB_EXPORT_DEFN CanStatistics();

	void beginNewRun();
	void computeDerived(unsigned int baudRate);

	//! dataLength is the user data size (DLC field)
	void onTransmit(unsigned int dataLength);

	//! dataLength is the user data size (DLC field)
	void onReceive(unsigned int dataLength);

	unsigned int totalTransmitted() { return m_totalTransmitted.load(); }
	unsigned int totalReceived() { return m_totalReceived.load(); }
	float txRate() { return m_internals.m_transmittedPerSec; }
	float rxRate() { return m_internals.m_receivedPerSec; }
	float busLoad() { return m_internals.m_busLoad; }


	double timeSinceReceived() {
		m_hrnow = high_resolution_clock::now();
		duration<double, micro> time_span = duration_cast<duration<double, micro>>(m_hrnow - m_hrreceived);
		return ( time_span.count() / 1000 );
	}
	double timeSinceTransmitted() {
		m_hrnow = high_resolution_clock::now();
		duration<double, micro> time_span = duration_cast<duration<double, micro>>(m_hrnow - m_hrtransmitted);
		return ( time_span.count() / 1000 );
	}
	double timeSinceOpened() {
		m_hrnow = high_resolution_clock::now();
		duration<double, micro> time_span = duration_cast<duration<double, micro>>(m_hrnow - m_hropen);
		return ( time_span.count() / 1000 );
	}
	void setTimeSinceOpened()      { m_hropen        = high_resolution_clock::now();	}
	void setTimeSinceReceived()    { m_hrreceived    = high_resolution_clock::now();	}
	void setTimeSinceTransmitted() { m_hrtransmitted = high_resolution_clock::now();	}

	void operator=(const CanStatistics & other);  // not default, because of atomic data

private:

	/* Fields below may be accessed by many threads at once */
	std::atomic_uint_least32_t m_totalTransmitted;
	std::atomic_uint_least32_t m_totalReceived;
	//! Number of messaged in current statistic run
	std::atomic_uint_least32_t m_transmitted;
	//! Number of messaged in current statistic run
	std::atomic_uint_least32_t m_received;


	std::atomic_uint_least32_t m_transmittedOctets;
	std::atomic_uint_least32_t m_receivedOctets;

	high_resolution_clock::time_point m_hrnow, m_hrreceived, m_hrtransmitted, m_hropen;

	//! Following is encapsulated as a class, to provide sane copying in assignment operator
	class Internals
	{
	public:
		float m_transmittedPerSec;
		float m_receivedPerSec;
		//! Bus load derived from #TX and #RX packages
		float m_busLoad;
		system_clock::time_point m_observationStart;
	};
	Internals m_internals;

};

}
#endif /* CANINTERFACE_SOCKCAN_CANSTATISTICS_H_ */
