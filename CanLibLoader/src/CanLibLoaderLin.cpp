#include "CanLibLoaderLin.h"
#include <string>
#include <stdexcept>
#include <sstream>
#include <boost/filesystem.hpp>
#include <LogIt.h>

#include "dlfcn.h"
//typedef void* (*f_CCanAccess)();

namespace CanModule
{

typedef CCanAccess* f_canAccess();

CanLibLoaderLin::CanLibLoaderLin(const std::string& libName)
: CanLibLoader(libName), p_dynamicLibrary(0)
{
	dynamicallyLoadLib(libName);
	createCanAccess();
}
CanLibLoaderLin::~CanLibLoaderLin() {
	dlclose(p_dynamicLibrary);
}

void CanLibLoaderLin::dynamicallyLoadLib(const std::string& libName)
{
	LOG(Log::DBG) << "Proceeding to load library " << libName;
	//The library is loaded through dlopen (Linux API)
	std::ostringstream ss;
	ss << "lib" << libName << "can.so";
	p_dynamicLibrary = dlopen(ss.str().c_str(), RTLD_NOW);
	//We check for errors while loading the library
	if (p_dynamicLibrary == 0)
	{
		char *err = dlerror();
		if (err) {
			std::ostringstream msg;
			msg << "Error: could not load dynamic library ["<<ss.str()<<"], current working directory ["<<boost::filesystem::current_path()<<"] error: "<<err;
			LOG(Log::ERR) << msg.str();
			throw std::runtime_error(msg.str());
		}
	}	
}

CCanAccess*  CanLibLoaderLin::createCanAccess()
{
	//Once the library is loaded, the resolve the function getHalAccess, which will give us an instance of the wanted object
	f_canAccess *canAccess;
	canAccess = (f_canAccess*)dlsym(p_dynamicLibrary,"getCanBusAccess");
	//We check for errors again. If there is an error the library is released from memory.
	char *err = dlerror();
	if (err) {
		LOG(Log::ERR) << "Error: could not locate the function, error: [" << err << "]";
		throw std::runtime_error("Error: could not locate the function");//TODO: add library name to message
	}
	if (!canAccess)
	{
		dlclose(p_dynamicLibrary);
		LOG(Log::ERR) << "Error: could not locate the function, error: [" << err << "]";
		throw std::runtime_error("Error: could not locate the function");//TODO: add library name to message	
	}
	//We call the function getHalAccess we got from the library. This will give us a pointer to an object, which we store.
//	m_canAccessInstance = (CCanAccess*)(canAccess());
      return canAccess();
}
}
