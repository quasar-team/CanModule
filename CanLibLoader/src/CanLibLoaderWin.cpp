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

	/**
	 * Retrieve the system error message for the last-error code
	 * this seems to return error code 0 even if lib pointer is NULL
	 */
	void CanLibLoaderWin::ErrorExit(LPTSTR lpszFunction)
	{
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
		LOG(Log::TRC, lh) << "inherited logItComponentHandle= " << lh;
		dynamicallyLoadLib(libName);
	}

	CanLibLoaderWin::~CanLibLoaderWin() {
		FreeLibrary(m_pDynamicLibrary);
	}
	/**
	 * The dll is loaded trough LoadLibrary (Windows API)
	 * but error reporting is poor. Use extended API method for this to get
	 * around the signing at least.
	 * ===AnaGate libs are NOT INSTALLED on the system, so make sure the vendor libs in question are
	 * in the search path: copy them to the bin exec dir.
	 * ===Systec libs are in fact installed into the system C:\Windows\system32\USBCAN64.dll or so, so
	 * for systec nothing has to be done. In fact this is unclean, but that is what they deliver.
	 */
	void CanLibLoaderWin::dynamicallyLoadLib(const std::string& libName)
	{
		std::ostringstream ss;
		ss << libName << "can.dll";

		//LOG(Log::DBG) << "Proceeding to load library " << ss.str();
		//m_pDynamicLibrary = ::LoadLibrary(ss.str().c_str());

		LOG(Log::DBG, lh) << "Proceeding to ExA load library " << ss.str();
		m_pDynamicLibrary = ::LoadLibraryExA(ss.str().c_str(), NULL, 0x00000010 /* LOAD_IGNORE_CODE_AUTHZ_LEVEL */);

		//We check for errors while loading the library
		if ( m_pDynamicLibrary != NULL )  {
			LOG(Log::DBG, lh) << " loaded the dynamic library: [" << ss.str() << "]";
		} else {
			string msg = string(__FUNCTION__) + string("Error: could not load the dynamic library ") + ss.str();
			LOG(Log::ERR, lh) << msg;
			if ( ss.str() == "ancan.dll"){
				LOG(Log::ERR, lh) << " WARNING: anagate vendor libs do not install on your system, they are just copied. Make sure the hidden dependend libs (i.e. AnaGateCan64.dll) are in your lib search path!";
				LOG(Log::ERR, lh) << " WARNING: easiest solution: copy them into the same directory as the CANX-tester binary";
			}
			ErrorExit(TEXT( "Error: could not load the dynamic library " ));
			throw std::runtime_error( msg.c_str() );
		}
	}

	/**
	 * Once the library is loaded, resolve the function getHalAccess,
	 * which will give us an instance of the wanted object: "access"
	 * One access instance corresponds to one CAN port (and whatever is
	 * needed on the computer side: ethernet, USB, whatnot)
	 */
	CanModule::CCanAccess* CanLibLoaderWin::createCanAccess()
	{
		LOG(Log::TRC, lh) << __FUNCTION__ << ": Accessing method get getCanBusAccess";
		f_CCanAccess *canAccess = (f_CCanAccess *)GetProcAddress(m_pDynamicLibrary, "getCanBusAccess");

		std::cout << __FILE__ << " " << __LINE__ << " " << __LINE__ << " CanAccess= 0x" << canAccess << std::endl;


		// We check for errors again. If there is an error the library is released from memory.
		if (!canAccess) {
			throw std::runtime_error( string(__FUNCTION__) + string(": Error: could not locate the function"));
		}
		// We call the function getHalAccess we got from the library. This will give us a pointer to an object, wich we store.
		LOG(Log::TRC, lh) << __FUNCTION__ << ": getCanBusAccess: got an object ptr from library, OK";


		return (CCanAccess*)(canAccess());
	}
}
