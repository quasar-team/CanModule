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
#include <errno.h>
#include <string.h>
#include <chrono>

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
	timeval convertTimepointToTimeval(const std::chrono::system_clock::time_point &t1)
#endif
	{
		timeval dest;
		auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(t1.time_since_epoch());
		dest.tv_sec = millisecs.count() / 1000;
		dest.tv_usec = (millisecs.count() % 1000) * 1000;
		return dest;
	}

	std::chrono::system_clock::time_point convertTimevalToTimepoint(const timeval &t1)
	{
		auto d = std::chrono::seconds(t1.tv_sec) + std::chrono::nanoseconds(t1.tv_usec);
		std::chrono::system_clock::time_point tp(std::chrono::duration_cast<std::chrono::system_clock::duration>(d));
		return tp;
	}

	// windows implementation errors on chrono
#ifdef _WIN32
	std::chrono::steady_clock::time_point currentTimeTimeval()	{ return std::chrono::ssteady_clock::now();}
#else
	std::chrono::system_clock::time_point currentTimeTimeval()	{ return std::chrono::system_clock::now();}
#endif

}
