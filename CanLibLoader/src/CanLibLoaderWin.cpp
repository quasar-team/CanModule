#include "CanLibLoaderWin.h"
#include <string>
#include <stdexcept>
#include <sstream>
#include "LogIt.h"

namespace CanModule
{

	typedef __declspec(dllimport) CCanAccess *f_CCanAccess();

	CanLibLoaderWin::CanLibLoaderWin(const std::string& libName)
		: CanLibLoader(libName), m_pDynamicLibrary(0)
	{
		dynamicallyLoadLib(libName);
//		createCanAccess();
	}

	CanLibLoaderWin::~CanLibLoaderWin() {
		FreeLibrary(m_pDynamicLibrary);
	}
	void CanLibLoaderWin::dynamicallyLoadLib(const std::string& libName)
	{
		LOG(Log::DBG) << "Proceeding to load library " << libName;
		std::ostringstream ss;
		ss << libName << "can.dll";
		//The dll is loaded trough LoadLibrary (Windows API)
		m_pDynamicLibrary = ::LoadLibrary(ss.str().c_str());
		//We check for errors while loading the library
		if (!m_pDynamicLibrary) {
			LOG(Log::ERR) << "Error: could not load the dynamic library: [" << ss.str() << "]";
			throw std::runtime_error("Error: could not load the dynamic library");//TODO: add library name to message
		}
	}
	CanModule::CCanAccess* CanLibLoaderWin::createCanAccess()
	{
		//Once the library is loaded, the resolve the function getHalAccess, which will give us an instance of the wanted object
		LOG(Log::TRC) << "Accesing method get getCanBusAccess";
		f_CCanAccess *canAccess = (f_CCanAccess *)GetProcAddress(m_pDynamicLibrary, "getCanBusAccess");
		//We check for errors again. If there is an error the library is released from memory.
		if (!canAccess) {
			throw std::runtime_error("Error: could not locate the function");
		}
		//We call the function getHalAccess we got from the library. This will give us a pointer to an object, wich we store.
		return (CCanAccess*)(canAccess());
	}
}