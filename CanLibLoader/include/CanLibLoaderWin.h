/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanLibLoaderWin.h
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
