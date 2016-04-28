/*
 * Utils.cpp
 *
 *  Created on: Nov 24, 2014
 *      Author: pnikiel
 */


#include <CCCUtils.h>
#include <errno.h>
#include <string.h>

std::string CCCerrnoToString( )
{
	const int max_len = 1000;
	char buf [max_len];
#ifdef _WIN32
	strerror_s(buf, max_len - 1, errno);
	return std::string(buf);
#else
	return std::string( strerror_r( errno, buf, max_len-1 ) );
#endif
}

double CCCsubtractTimeval (const timeval &t1, const timeval &t2)
{

	return t2.tv_sec-t1.tv_sec + double(t2.tv_usec-t1.tv_usec)/1000000.0;

}
