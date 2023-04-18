/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanModuleUtils.cpp
 *
 *  Created on: Nov 24, 2014
 *      Author: pnikiel * CanBusAccess.cpp
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
#include <CCanAccess.h>

#include <errno.h>
#include <chrono>
#include <iostream>
#include <sstream>

namespace CanModule
{


	std::string CanModuleerrnoToString()
	{
		const int max_len = 1000;
		char buf[max_len];
#ifdef _WIN32
		strerror_s(buf, max_len - 1, errno);
		return std::string(buf);
#else
		return std::string(strerror_r(errno, buf, max_len - 1));
#endif
	}


	// windows implementation errors on chrono
#ifdef _WIN32
	timeval convertTimepointToTimeval(const std::chrono::steady_clock::time_point &t1)
#else
	timeval convertTimepointToTimeval(const std::chrono::high_resolution_clock::time_point &t1)
#endif
	{
		timeval dest;
		auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(t1.time_since_epoch());
		dest.tv_sec = millisecs.count() / 1000;
		dest.tv_usec = (millisecs.count() % 1000) * 1000;
		return dest;
	}

#ifdef _WIN32
	std::chrono::steady_clock::time_point convertTimevalToTimepoint(const timeval &t1)
	{
		auto d = std::chrono::seconds(t1.tv_sec) + std::chrono::nanoseconds(t1.tv_usec);
		std::chrono::steady_clock::time_point tp(std::chrono::duration_cast<std::chrono::steady_clock::duration>(d));
		return tp;
	}
#else
	std::chrono::high_resolution_clock::time_point convertTimevalToTimepoint(const timeval &t1)
	{
		auto d = std::chrono::seconds(t1.tv_sec) + std::chrono::nanoseconds(t1.tv_usec);
		std::chrono::high_resolution_clock::time_point tp(std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(d));
		return tp;
	}
#endif

	// windows implementation errors on chrono
#ifdef _WIN32
	std::chrono::steady_clock::time_point currentTimeTimeval()	{ return std::chrono::steady_clock::now();}
#else
	std::chrono::high_resolution_clock::time_point currentTimeTimeval()	{ return std::chrono::high_resolution_clock::now();}
#endif

	std::string canMessageToString(CanMessage &f)
	{
		std::string result;
		result =  "[id=0x"+CanModuleUtils::toHexString(f.c_id, 3, '0')+" ";
		if (f.c_id & f.c_rtr)
			result += "RTR ";
		result+="dlc=" + CanModuleUtils::toString(int(f.c_dlc)) + " data=[";

		for (int i=0; i<f.c_dlc; i++)
			result+= CanModuleUtils::toHexString((unsigned int)f.c_data[i], 2, '0')+" ";
		result += "]]";
		return result;
	}

	std::string canMessage2ToString(short cobID, unsigned char len, unsigned char *message, bool rtr)
	{
		std::string result = "";
		result =  "[id=0x"+CanModuleUtils::toHexString(cobID, 3, '0')+" ";
		if ( rtr )
			result += "RTR ";
		result+="dlc= " + CanModuleUtils::toHexString( len ) + " (hex) data= ";

		for (int i=0; i< len; i++)
			result+= CanModuleUtils::toHexString((unsigned int) message[i], 2, '0')+" ";
		result += "]";
		return result;
	}


	/**
	 * translates the value of enum CanModule_bus_state  to a text, applicable for all vendors
	 * you can also throw any (int) value at it, that will be reported as an unknown state, but still reported
	 */
	/* static */ std::string translateCanBusStateToText( CanModule::CanModule_bus_state state )
	{
		switch (state)
		{
		case CanModule::CAN_STATE_ERROR_ACTIVE: return " CAN_STATE_ERROR_ACTIVE: RX/TX error count < 96  (OK) ";
		case CanModule::CAN_STATE_ERROR_WARNING: return " CAN_STATE_ERROR_WARNING: RX/TX error count < 128 ";
		case CanModule::CAN_STATE_ERROR_PASSIVE: return " CAN_STATE_ERROR_PASSIVE: RX/TX error count < 256 ";
		case CanModule::CAN_STATE_BUS_OFF: return " CAN_STATE_BUS_OFF: RX/TX error count >= 256 ";
		case CanModule::CAN_STATE_STOPPED: return " CAN_STATE_STOPPED: Device is stopped ";
		case CanModule::CAN_STATE_SLEEPING: return " CAN_STATE_SLEEPING: Device is sleeping ";
		case CanModule::CAN_STATE_MAX: return " CAN_STATE_MAX: Rx or Tx queue full, overrun ";

		case CanModule::CANMODULE_NOSTATE: return " CANMODULE_NOSTATE: could not get port state ";
		case CanModule::CANMODULE_WARNING: return " CANMODULE_WARNING: traffic degradation but might recover ";
		case CanModule::CANMODULE_ERROR: return " CANMODULE_ERROR: error likely stemming from SW/HW/firmware ";
		case CanModule::CANMODULE_TIMEOUT_OK: return " CANMODULE_TIMEOUT_OK: bus is fine, no traffic ";
		case CanModule::CANMODULE_OK: return " CANMODULE_OK: bus is fine ";

		default:
			std::stringstream os;
			os <<  " translateCanBusStateToText: (very bad) unknown state= " << state;
			return ( os.str() );
			// dont throw an exception
		}
	}


}
