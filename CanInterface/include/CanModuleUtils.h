/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanModuleUtils.h
 *
 *  Created on: Oct 22, 2014
 *      Author: Piotr Nikiel <piotr@nikiel.info>
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
 */

#ifndef CanModuleUTILS_H_
#define CanModuleUTILS_H_

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/time.h>
#endif
#include <sstream>
#include <stdexcept>
#include <string>
#include <chrono>

#include "CanMessage.h"

namespace CanModule
{
	class CanModuleUtils
	{
	public:

		template<typename T>
		static std::string toString(const T t)
		{
			std::ostringstream oss;
			oss << t;
			return oss.str();
		}

		template<typename T>
		static std::string toHexString(const T t, unsigned int width = 0, char zeropad = ' ')
		{
			std::ostringstream oss;
			oss << std::hex;
			if (width > 0)
			{
				oss.width(width);
				oss.fill(zeropad);
			}
			oss << (unsigned long)t << std::dec;
			return oss.str();
		}

		static unsigned int fromHexString(const std::string &s)
		{
			unsigned int x;
			std::istringstream iss(s);
			iss >> std::hex >> x;
			if (iss.bad())
				throw std::runtime_error("Given string '" + s + "' not convertible from hex to uint");
			return x;
		}

	//	static  std::string translateCanBusStateToText( CanModule::CanModuleUtils::CanModule_bus_state state );

	};


#ifdef _WIN32
	timeval convertTimepointToTimeval(const std::chrono::steady_clock::time_point &t1);
	std::chrono::steady_clock::time_point convertTimevalToTimepoint(const timeval &t1);
	std::chrono::steady_clock::time_point currentTimeTimeval();
#else
	timeval convertTimepointToTimeval(const std::chrono::system_clock::time_point &t1);
	std::chrono::system_clock::time_point convertTimevalToTimepoint(const timeval &t1);
	std::chrono::system_clock::time_point currentTimeTimeval();
#endif

	/*
	 * CAN operational and error states, copied from linux can_netlink.h can_state and extended.
	 *
	 * Lets try to have all vendors look like that or like a subset of that.
	 * The CAN_STATE keep their original definitions and semantics, so if the enum is set to one of these values
	 * they are reported as that from the vendor.
	 *
	 * The vendors which do not fully implement that take values CANMODULE_ instead. One can have a mix if
	 * a vendor e.g. implements only CAN_STATE_ERROR_ACTIVE but not the rest.
	 *
	 */
	enum CanModule_bus_state {
		// CAN model i.e. https://www.csselectronics.com/pages/can-bus-errors-intro-tutorial. guaranteed to keep the meaning and semantics
		// if they are set
		CAN_STATE_ERROR_ACTIVE = 0,	/* RX/TX error count < 96 */
		CAN_STATE_ERROR_WARNING,	/* RX/TX error count < 128 */
		CAN_STATE_ERROR_PASSIVE,	/* RX/TX error count < 256 */
		CAN_STATE_BUS_OFF,		/* RX/TX error count >= 256 */
		CAN_STATE_STOPPED,		/* Device is stopped */
		CAN_STATE_SLEEPING,		/* Device is sleeping */
		CAN_STATE_MAX,

		// CanModule extension, to cover non-socketcan implementations
		CANMODULE_NOSTATE,   // could not get state
		CANMODULE_WARNING,   // degradation but might recover
		CANMODULE_ERROR,     // error likely stemming from SW/HW/firmware
		CANMODULE_TIMEOUT_OK, // bus is fine, no traffic
		CANMODULE_OK         // bus is fine
	};



	std::string CanModuleerrnoToString();
	std::string canMessageToString(CanMessage &f);
	std::string canMessage2ToString(short cobID, unsigned char len, unsigned char *message, bool rtr);
	std::string translateCanBusStateToText( CanModule::CanModule_bus_state state ); // static public in-class declaration does not work for windows
}
#endif /* UTILS_H_ */
