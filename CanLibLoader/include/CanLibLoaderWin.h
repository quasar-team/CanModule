#pragma once
#define _WINSOCKAPI_
#include <windows.h>
#include <strsafe.h>

#include "CanLibLoader.h"

namespace CanModule
{
	class CanLibLoaderWin : public CanLibLoader
	{
	public:
		// system error reporter
		void ErrorExit(LPTSTR lpszFunction);
		//Empty constructor
		CanLibLoaderWin(const std::string& libName);
		//Will cleanup the loaded dynamic library
		virtual ~CanLibLoaderWin();
	protected:
		//Load a dynamic library.
		virtual void dynamicallyLoadLib(const std::string& libName);
		//Uses the loaded library to create a HAL object and store it in p_halInstance
		virtual  CCanAccess* createCanAccess();
	private:
		//Pointer to the dynamic library stored on the memory
		HMODULE m_pDynamicLibrary;
	};
}
