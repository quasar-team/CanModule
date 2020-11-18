/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanLibLoader.h
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
#include <string>
#include <map>
#include "CCanAccess.h"
#include "ExportDefinition.h"

namespace CanModule 
{

class CanLibLoader 	{

protected:
	//Empty constructor
	CanLibLoader(const std::string& libName);

public:
	//Will cleanup the loaded dynamic library
	virtual ~CanLibLoader();
	SHARED_LIB_EXPORT_DEFN static CanLibLoader* createInstance(const std::string& libName);

	SHARED_LIB_EXPORT_DEFN CanModule::CCanAccess * openCanBus(std::string name, std::string parameters);
	SHARED_LIB_EXPORT_DEFN	void closeCanBus(CanModule::CCanAccess *cca);

	// LogIt handle
	Log::LogComponentHandle lh;

	void setLibName( std::string ln ){ m_libName = ln; }
	std::string getLibName(){ return (m_libName);}


protected:
	//Load a dynamic library.
	virtual void dynamicallyLoadLib(const std::string& libName) = 0;
	//Uses the loaded library to create a HAL object and store it in p_halInstance
	virtual CCanAccess* createCanAccess() = 0;

private:
	std::string m_libName;

};
}
