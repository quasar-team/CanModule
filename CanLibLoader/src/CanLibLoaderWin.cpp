#include "CanLibLoaderWin.h"
#include <string>
#include <stdexcept>
#include <sstream>
#include "LogIt.h"

using namespace CanModule;

typedef __declspec(dllimport) CCanAccess *f_CCanAccess();

CanLibLoaderWin::CanLibLoaderWin(const std::string& libName)
: CanLibLoader(libName), p_dynamicLibrary(0)
{
	dynamicallyLoadLib(libName);
	createCanAccess();
}

CanLibLoaderWin::~CanLibLoaderWin() {
	FreeLibrary(p_dynamicLibrary);
}
void CanLibLoaderWin::dynamicallyLoadLib(const std::string& libName)
{
	LOG(Log::DBG) << "Proceeding to load library " << libName;
	std::ostringstream ss;
	ss << libName << ".dll";
	//The dll is loaded trough LoadLibrary (Windows API)
	p_dynamicLibrary = ::LoadLibrary(ss.str().c_str());
	//We check for errors while loading the library
	if (!p_dynamicLibrary) {
		LOG(Log::ERR) << "Error: could not load the dynamic library: [" << ss.str() << "]";
		throw std::runtime_error("Error: could not load the dynamic library");//TODO: add library name to message
	}
}
void CanLibLoaderWin::createCanAccess()
{
	//Once the library is loaded, the resolve the function getHalAccess, which will give us an instance of the wanted object
	LOG(Log::TRC) << "Accesing method get getCanbusAccess";
	f_CCanAccess *canAccess = (f_CCanAccess *)GetProcAddress(p_dynamicLibrary, "getCanbusAccess");
	LOG(Log::TRC) << "Done!";
	//We check for errors again. If there is an error the library is released from memory.
	if (!canAccess) {
		throw std::runtime_error("Error: could not locate the function");
	}
	//We call the function getHalAccess we got from the library. This will give us a pointer to an object, wich we store.
	m_canAccessInstance = (CCanAccess*)(canAccess());
}
