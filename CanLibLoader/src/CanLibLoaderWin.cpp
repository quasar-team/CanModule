/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanLibLoaderWin.cpp
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
		LOG(Log::TRC) << "CanLibLoaderWin::createCanAccess: Accessing method get getCanBusAccess";
		f_CCanAccess *canAccess = (f_CCanAccess *)GetProcAddress(m_pDynamicLibrary, "getCanBusAccess");
		//We check for errors again. If there is an error the library is released from memory.
		if (!canAccess) {
			throw std::runtime_error("CanLibLoaderWin::createCanAccess: Error: could not locate the function");
		}
		//We call the function getHalAccess we got from the library. This will give us a pointer to an object, wich we store.
		LOG(Log::TRC) << "CanLibLoaderWin::createCanAccess: getCanBusAccess: got an object ptr from library, OK";
		return (CCanAccess*)(canAccess());
	}
}
