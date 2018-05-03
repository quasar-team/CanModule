/*
 * Utils.cpp
 *
 *  Created on: Nov 24, 2014
 *      Author: pnikiel
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

	timeval convertTimepointToTimeval(const std::chrono::system_clock::time_point &t1)
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

	std::chrono::system_clock::time_point currentTimeTimeval()
	{
	//	timeval ftTimeStamp;

		auto now = std::chrono::system_clock::now();
		/*
		auto nMicrosecs =
			std::chrono::duration_cast<std::chrono::microseconds>(
				now.time_since_epoch()
					);
			ftTimeStamp.tv_sec = nMicrosecs.count() / 1000000L;
			ftTimeStamp.tv_usec = (nMicrosecs.count() % 1000000L);
			*/
			return now;
			
	}
	double CanModulesubtractTimeval(const std::chrono::system_clock::time_point &t1, const std::chrono::system_clock::time_point &t2)
	{
		std::chrono::duration<double> differ = t2 - t1;
		return differ.count();

	}

	/*
	double CanModulesubtractTimeval(const timeval &t1, const timeval &t2)
	{

		return t2.tv_sec - t1.tv_sec + double(t2.tv_usec - t1.tv_usec) / 1000000.0;

	}
	*/
}
