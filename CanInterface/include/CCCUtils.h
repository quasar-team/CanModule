/* © Copyright CERN, 2015. All rights not expressly granted are reserved. */
/*
 * Utils.h
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

#ifndef CCCUTILS_H_
#define CCCUTILS_H_

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/time.h>
#endif
#include <sstream>
#include <stdexcept>
#include <string>
//#include <uadatetime.h>

class CCCUtils
{
public:
template<typename T>
static std::string toString (const T t)
{
        std::ostringstream oss;
        oss << t;
        return oss.str ();
}

template<typename T>
static std::string toHexString (const T t, unsigned int width=0, char zeropad=' ')
{
        std::ostringstream oss;
        oss << std::hex;
        if (width > 0)
        {
        	oss.width(width);
        	oss.fill(zeropad);
        }
        oss << (unsigned long)t << std::dec;
        return oss.str ();
}

static unsigned int fromHexString (const std::string &s)
{
	unsigned int x;
	std::istringstream iss (s);
	iss >> std::hex >> x;
	if (iss.bad())
		throw std::runtime_error ("Given string '"+s+"' not convertible from hex to uint");
	return x;
}

};

double CCCsubtractTimeval (const timeval &t1, const timeval &t2);

//UaString bytesToUaString( const unsigned char* data, unsigned int len );

std::string CCCerrnoToString( );

#endif /* UTILS_H_ */
