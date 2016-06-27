/*
 * InterfaceStatistics.h
 *
 *  Created on: Mar 17, 2015
 *      Author: pnikiel
 */

#ifndef CANINTERFACE_SOCKCAN_CANSTATISTICS_H_
#define CANINTERFACE_SOCKCAN_CANSTATISTICS_H_


#ifdef _WIN32
#include <atomic>
#include <time.h>
#ifndef timeval
#include "Winsock2.h"
#endif
#else
#include <stdatomic.h>
#include <sys/time.h>
#endif

class CanStatistics
{


public:

	CanStatistics();

	void beginNewRun();
	void computeDerived (unsigned int baudRate);

	//! dataLength is the user data size (DLC field)
	void onTransmit( unsigned int dataLength );

	//! dataLength is the user data size (DLC field)
	void onReceive( unsigned int dataLength);

	unsigned int totalTransmitted() { return m_totalTransmitted.load(); }
	unsigned int totalReceived() { return m_totalReceived.load(); }
	float txRate() { return m_internals.m_transmittedPerSec; }
	float rxRate() { return m_internals.m_receivedPerSec; }
	float busLoad() { return m_internals.m_busLoad; }

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

	//! Following is encapsulated as a class, to provide sane copying in assignment operator
	class Internals
	{
	public:
		float m_transmittedPerSec;
		float m_receivedPerSec;
		//! Bus load derived from #TX and #RX packages
		float m_busLoad;
		timeval m_observationStart;
	};
	Internals m_internals;

};


#endif /* CANINTERFACE_SOCKCAN_CANSTATISTICS_H_ */
