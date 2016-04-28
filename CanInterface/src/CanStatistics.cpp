/*
 * InterfaceStatistics.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: pnikiel
 */

#include <CCCUtils.h>
#include "CanStatistics.h"
#ifdef _WIN32
#include "gettimeofday.h"
#endif
#include <iostream>

CanStatistics::CanStatistics():
	m_totalTransmitted(0),
	m_totalReceived(0),
	m_transmitted(0),
	m_received(0),
	m_transmittedOctets(0),
	m_receivedOctets(0)
{

}

void CanStatistics::beginNewRun()
{
	gettimeofday( &m_internals.m_observationStart, 0 );
	m_transmitted = 0;
	m_received = 0;
	m_transmittedOctets = 0;
	m_receivedOctets = 0;
}

void CanStatistics::computeDerived (unsigned int baudRate)
{
	timeval tnow;
	gettimeofday( &tnow, 0 );
	double period = CCCsubtractTimeval( m_internals.m_observationStart, tnow );
	m_internals.m_transmittedPerSec = m_transmitted / period;
	m_internals.m_receivedPerSec = m_received / period;

	if (baudRate>0)
	{ // baud rate is known
		unsigned int octets = m_transmittedOctets.load() + m_receivedOctets.load();
		float maxOctetsInSecond = (float)baudRate / 8.0;
		m_internals.m_busLoad = octets / maxOctetsInSecond;
	}
	else
		m_internals.m_busLoad = 0;




}

void CanStatistics::onTransmit( unsigned int dataLength )
{
	m_totalTransmitted++;
	m_transmitted++;
	m_transmittedOctets += 2 + 1 + dataLength + 2; /* ID, DLC, USER DATA, CRC */

}
void CanStatistics::onReceive(  unsigned int dataLength )
{
	m_totalReceived++;
	m_received++;
	m_receivedOctets += 2 + 1 + dataLength + 2; /* ID, DLC, USER DATA, CRC */
}

void CanStatistics::operator=(const CanStatistics & other)
{

	this->m_received = other.m_received.load();
	this->m_totalReceived = other.m_totalReceived.load();
	this->m_transmitted = other.m_transmitted.load();
	this->m_totalTransmitted = other.m_totalTransmitted.load();
	this->m_transmittedOctets = other.m_transmittedOctets.load();
	this->m_receivedOctets = other.m_receivedOctets.load();
	this->m_internals = other.m_internals;
}
