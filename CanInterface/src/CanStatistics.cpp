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

#include <CanModuleUtils.h>
#include "CanStatistics.h"

#include <iostream>


namespace CanModule
{
	CanStatistics::CanStatistics() :
		m_totalTransmitted(0),
		m_totalReceived(0),
		m_transmitted(0),
		m_received(0),
		m_transmittedOctets(0),
		m_receivedOctets(0),
		m_portStatus(0)
	{}

	void CanStatistics::beginNewRun()
	{
		m_internals.m_observationStart = std::chrono::system_clock::now();
		m_transmitted = 0;
		m_received = 0;
		m_transmittedOctets = 0;
		m_receivedOctets = 0;
	}

	/**
	 * encode a port status from the various informations of the CAN system
	 *
	 * the whole netlink structs are available. We can code anything as long as
	 * it is usefully making sense for all vendors. see void CSockCanScan::updateBusStatus(){
	 * but lets just copy the _state for now, which is a simple enum:
	 *
	 * enum can_state {
	 * 	CAN_STATE_ERROR_ACTIVE = 0,	 RX/TX error count < 96
	 * 	CAN_STATE_ERROR_WARNING,	 RX/TX error count < 128
	 * 	CAN_STATE_ERROR_PASSIVE,	 RX/TX error count < 256
	 * 	CAN_STATE_BUS_OFF,		 RX/TX error count >= 256
	 * 	CAN_STATE_STOPPED,		 Device is stopped
	 * 	CAN_STATE_SLEEPING,		 Device is sleeping
	 * 	CAN_STATE_MAX
	 * 	};
	 */
	void CanStatistics::encodeCanModuleStatus(){
		m_portStatus = m_internals.m_state;
	}

	void CanStatistics::computeDerived(unsigned int baudRate)
	{
		system_clock::time_point tnom = system_clock::now(); 
		const std::chrono::time_point<std::chrono::system_clock> now =
		        std::chrono::system_clock::now();

		auto nDiff = tnom - m_internals.m_observationStart;
		auto period = duration_cast<seconds>(nDiff).count();

		std::cout<< __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " period= "
				<< period << std::endl;

		m_internals.m_transmittedPerSec = (period != 0 ? float(m_transmitted / period) : 0);
		m_internals.m_receivedPerSec = (period != 0 ? float(m_received / period) : 0);

		if (baudRate > 0)
		{ // baud rate is known
			unsigned int octets = m_transmittedOctets.load() + m_receivedOctets.load();
			float maxOctetsInSecond = float(baudRate / 8.0);
			m_internals.m_busLoad = float(octets / maxOctetsInSecond);
		} else {
			m_internals.m_busLoad = 0;
		}
	}

	void CanStatistics::onTransmit(unsigned int dataLength)
	{
		m_totalTransmitted++;
		m_transmitted++;
		m_transmittedOctets += 2 + 1 + dataLength + 2; /* ID, DLC, USER DATA, CRC */
	}

	void CanStatistics::onReceive(unsigned int dataLength)
	{
		m_totalReceived++;
		m_received++;
		m_receivedOctets += 2 + 1 + dataLength + 2; /* ID, DLC, USER DATA, CRC */
#ifdef _WIN32
		GetSystemTime(&m_dreceived);
#else
		gettimeofday( &m_dreceived, &m_tz);
#endif
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

		this->m_dreceived = other.m_dreceived;
		this->m_dtransmitted = other.m_dtransmitted;
		this->m_dopen = other.m_dopen;
		this->m_now = other.m_now;

		this->m_portStatus = other.m_portStatus;
	}
}
