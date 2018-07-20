#include "CanLibLoaderWin.h"
#include <string>
#include <stdexcept>
#include <sstream>
#include "LogIt.h"

namespace CanModule
{

	typedef __declspec(dllimport) CCanAccess *f_CCanAccess();

	// this seems to return error code 0 even if lib pointer is NULL
	void CanLibLoaderWin::ErrorExit(LPTSTR lpszFunction)
	{
	    // Retrieve the system error message for the last-error code

	    LPVOID lpMsgBuf;
	    LPVOID lpDisplayBuf;
	    DWORD dw = GetLastError();

	    FormatMessage(
	        FORMAT_MESSAGE_ALLOCATE_BUFFER |
	        FORMAT_MESSAGE_FROM_SYSTEM |
	        FORMAT_MESSAGE_IGNORE_INSERTS,
	        NULL,
	        dw,
	        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	        (LPTSTR) &lpMsgBuf,
	        0, NULL );

	    // Display the error message and exit the process

	    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
	        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	    StringCchPrintf((LPTSTR)lpDisplayBuf,
	        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
	        TEXT("%s failed with error %d: %s"),
	        lpszFunction, dw, lpMsgBuf);
	    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	    LocalFree(lpMsgBuf);
	    LocalFree(lpDisplayBuf);
	    //ExitProcess(dw);
	}



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
		std::ostringstream ss;
		ss << libName << "can.dll";
		//The dll is loaded trough LoadLibrary (Windows API)
		LOG(Log::DBG) << "Proceeding to load library " << ss.str();
		m_pDynamicLibrary = ::LoadLibrary(ss.str().c_str());
		//We check for errors while loading the library
		if ( m_pDynamicLibrary != NULL )  {
			LOG(Log::DBG) << "loaded the dynamic library: [" << ss.str() << "]";
		} else {
			LOG(Log::ERR) << "could not load the dynamic library: [" << ss.str() << "]";
			// ErrorExit(TEXT("could not load the lib "));
			string msg = string("Error: could not load the dynamic library") + ss.str();
			throw std::runtime_error( msg.c_str() );
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
		LOG(Log::TRC) << "createCanAccess getCanBusAccess: got an object ptr from library, OK";
		return (CCanAccess*)(canAccess());
	}
}
