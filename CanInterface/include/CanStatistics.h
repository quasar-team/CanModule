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


#include "ExportDefinition.h"
#include <atomic>
#include <iostream>
#include <chrono>

template<class Duration>
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock, Duration>;

namespace CanModule
{
class CanStatistics
{

public:

	SHARED_LIB_EXPORT_DEFN CanStatistics();

	void beginNewRun();
	void computeDerived(unsigned int baudRate);

	/**
	 * called when transmitting a message
	 * dataLength is the user data size (DLC field)
	 */
	void onTransmit(unsigned int dataLength);

	/**
	 * called when receiving a message
	 * dataLength is the user data size (DLC field)
	 */
	void onReceive(unsigned int dataLength);

	unsigned int totalTransmitted() { return m_totalTransmitted.load(); }
	unsigned int totalReceived() { return m_totalReceived.load(); }

	/**
	 * transmission rate bytes/s
	 */
	float txRate() { return m_internals.m_transmittedPerSec; }

	/**
	 * reception rate bytes/s
	 */
	float rxRate() { return m_internals.m_receivedPerSec; }

	/**
	 * bus load in 0...1 ( % * 100 ) of max bus speed (baudrate)
	 */
	float busLoad() { return m_internals.m_busLoad; }

	/**
	 * time since the last message was received in microseconds precision
	 */
	double timeSinceReceived() {
		m_hrnow = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> time_span = m_hrnow - m_hrreceived;
		return ( time_span.count() / 1000 );
	}

	/**
	 * time since the last message was sent, in microseconds precision
	 */
	double timeSinceTransmitted() {
		m_hrnow = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> time_span = m_hrnow - m_hrtransmitted;
		return ( time_span.count() / 1000 );
	}

	/**
	 *  time since that bus was opened, in microseconds precision
	 */
	double timeSinceOpened() {
		m_hrnow = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> time_span = m_hrnow - m_hropen;
		return ( time_span.count() / 1000 );
	}
	void setTimeSinceOpened()      { m_hropen        = std::chrono::high_resolution_clock::now();	}
	void setTimeSinceReceived()    { m_hrreceived    = std::chrono::high_resolution_clock::now();	}
	void setTimeSinceTransmitted() { m_hrtransmitted = std::chrono::high_resolution_clock::now();	}

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

	std::chrono::high_resolution_clock::time_point m_hrnow, m_hrreceived, m_hrtransmitted, m_hropen;

	//! Following is encapsulated as a class, to provide sane copying in assignment operator
	class Internals
	{
	public:
		float m_transmittedPerSec;
		float m_receivedPerSec;
		//! Bus load derived from #TX and #RX packages
		float m_busLoad;
		std::chrono::system_clock::time_point m_observationStart;
	};
	Internals m_internals;

};

}
#endif /* CANINTERFACE_SOCKCAN_CANSTATISTICS_H_ */
