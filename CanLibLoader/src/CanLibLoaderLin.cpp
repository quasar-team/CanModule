/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanLibLoaderLin.cpp
 *
 *  Created on: Feb 22, 2012
 *      Author: vfilimon
 *      mludwig at cern dot ch
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
#include "CanLibLoaderLin.h"
#include <string>
#include <stdexcept>
#include <sstream>
#include <boost/filesystem.hpp>
#include <LogIt.h>

#include "dlfcn.h"


namespace CanModule
{

typedef CCanAccess* f_canAccess();

CanLibLoaderLin::CanLibLoaderLin(const std::string& libName)
: CanLibLoader(libName), p_dynamicLibrary(0)
{
	LOG(Log::TRC, lh) << "inherited logItComponentHandle= " << lh;
	m_gsig = GlobalErrorSignaler::getInstance();

	dynamicallyLoadLib(libName);
}

CanLibLoaderLin::~CanLibLoaderLin() {
	dlclose(p_dynamicLibrary);
}

void CanLibLoaderLin::dynamicallyLoadLib(const std::string& libName)
{
	// The library is loaded through dlopen (Linux API)
	std::ostringstream ss;
	ss << "lib" << libName << "can.so";
	std::cout  << "Proceeding to load library " << ss.str() << std::endl;
	LOG(Log::DBG, lh) << "Proceeding to load library " << ss.str();
	p_dynamicLibrary = dlopen(ss.str().c_str(), RTLD_NOW);

	// We check for errors while loading the library
	if (p_dynamicLibrary == 0){
		char *err = dlerror();
		if (err) {
			std::ostringstream msg;
			msg << "Error: could not load library ["<<ss.str()<<"], current working directory ["<<boost::filesystem::current_path()<<"] error: "<<err;
			LOG(Log::ERR, lh) << msg.str();

			m_gsig->fireSignal( 010, msg.str().c_str() );

			// this might be caught by the client, so we do not neccessarily stop execution here.
			throw std::runtime_error(msg.str());
		}
	} else {
		LOG(Log::DBG, lh) << ss.str() << " is loaded OK";
	}
	setLibName(ss.str());
}

/**
 * Once the library is loaded, resolve the function getHalAccess,
 * which will give us an instance of the wanted object: "access"
 * One access instance corresponds to one CAN port (and whatever is
 * needed on the computer side: ethernet, USB, whatnot)
 */
CCanAccess*  CanLibLoaderLin::createCanAccess()
{
	f_canAccess *canAccess;
	canAccess = (f_canAccess*)dlsym(p_dynamicLibrary,"getCanBusAccess");
	// We check for errors again. If there is an error the library is released from memory.
	char *err = dlerror();
	if (err) {
		std::ostringstream msg;
		msg << "Error: could not locate the function 'getCanBusAccess', error: [" << err << "]";
		LOG(Log::ERR, lh) << msg.str();

		m_gsig->fireSignal( 011, msg.str().c_str() );

		throw std::runtime_error( msg.str() );
	}
	if (!canAccess)
	{
		dlclose(p_dynamicLibrary);
		std::ostringstream msg;
		msg << "Error: could not locate the function 'canAccess', error: [" << err << "]";
		LOG(Log::ERR, lh) << msg.str();

		m_gsig->fireSignal( 012, msg.str().c_str() );

		throw std::runtime_error( msg.str() );
	}
	// We call the function getHalAccess we got from the library. This will give us a pointer to an object, which we store.
	return canAccess();
}
}
