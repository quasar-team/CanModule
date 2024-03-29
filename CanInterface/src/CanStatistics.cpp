/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanModuleUtils.cpp
 *
 * CanStatistics.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: pnikiel
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
#include <iostream>

#include "CanModuleUtils.h"
#include "CanStatistics.h"

namespace CanModule
{
	CanStatistics::CanStatistics() :
		m_totalTransmitted(0),
		m_totalReceived(0),
		m_transmitted(0),
		m_received(0),
		m_transmittedOctets(0),
		m_receivedOctets(0)
	{}

	void CanStatistics::beginNewRun()
	{
		m_internals.m_observationStart = std::chrono::high_resolution_clock::now();
		m_transmitted = 0;
		m_received = 0;
		m_transmittedOctets = 0;
		m_transmittedBits = 0;
		m_receivedOctets = 0;
		m_receivedBits = 0;
	}
	void CanStatistics::computeDerived(unsigned int baudRate)
	{
		// todo: should this not be high_resultion_clock ?
		std::chrono::high_resolution_clock::time_point tnom = std::chrono::high_resolution_clock::now();
		auto nDiff = tnom - m_internals.m_observationStart;
		auto period = std::chrono::duration_cast<std::chrono::seconds>(nDiff).count();

		m_internals.m_transmittedPerSec = (period != 0 ? float(m_transmitted / period) : 0);
		m_internals.m_receivedPerSec = (period != 0 ? float(m_received / period) : 0);

		if (baudRate > 0)
		{
			unsigned int octets = m_transmittedOctets.load() + m_receivedOctets.load();
			float maxOctetsInSecond = float(baudRate / 8.0);
			m_internals.m_busLoad = float(octets / maxOctetsInSecond); // wrong

			float bitsPerSecond = (period != 0 ? (( m_transmittedBits.load() + m_receivedBits.load() ) / period) : 0);
			m_internals.m_hardwareBusLoad = bitsPerSecond / (float ) baudRate;
		} else {
			m_internals.m_busLoad = 0;
			m_internals.m_hardwareBusLoad = 0.0;
		}
	}

	void CanStatistics::onTransmit(unsigned int dataLength)
	{
		m_totalTransmitted++;
		m_transmitted++;
		m_transmittedOctets += 2 + 1 + dataLength + 2; /* ID, DLC, USER DATA, CRC */
		m_transmittedBits += 48 + dataLength * 8; // standard msg base frame
	}

	void CanStatistics::onReceive(unsigned int dataLength)
	{
		m_totalReceived++;
		m_received++;
		m_receivedOctets += 2 + 1 + dataLength + 2; /* ID, DLC, USER DATA, CRC */
		m_receivedBits += 48 + dataLength * 8; // standard msg base frame
		setTimeSinceReceived();
	}

	void CanStatistics::operator=(const CanStatistics & other)
	{
		this->m_received = other.m_received.load();
		this->m_totalReceived = other.m_totalReceived.load();
		this->m_transmitted = other.m_transmitted.load();
		this->m_totalTransmitted = other.m_totalTransmitted.load();
		this->m_transmittedOctets = other.m_transmittedOctets.load();
		this->m_transmittedBits = other.m_transmittedBits.load();
		this->m_receivedOctets = other.m_receivedOctets.load();
		this->m_receivedBits = other.m_receivedBits.load();
		this->m_internals = other.m_internals;

		this->m_hrreceived = other.m_hrreceived;
		this->m_hrtransmitted = other.m_hrtransmitted;
		this->m_hropen = other.m_hropen;
		this->m_hrnow = other.m_hrnow;
	}
}
