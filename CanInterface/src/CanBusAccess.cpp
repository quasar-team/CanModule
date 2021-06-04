/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanBusAccess.cpp
 *
 *  Created on: Apr 4, 2011
 *      Author: vfilimon
 *      maintaining touches: mludwig, quasar team
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
 *
 */
#include "CanBusAccess.h"

namespace CanModule
{


void CanBusAccess::closeCanBus(CCanAccess *cInter)
{
	LOG(Log::INF) << __FUNCTION__ ;
	if ( cInter != 0 ){
		CanLibLoader *dlcan;
		map<string, CanLibLoader *>::iterator itcomponent;
		string bus = cInter->getBusName();
		string nameComponent = bus.substr(0, bus.find_first_of(':'));

		itcomponent = m_Component.find(nameComponent);
		dlcan = (*itcomponent).second;
		dlcan->closeCanBus(cInter);
	}
}

CCanAccess* CanBusAccess::openCanBus(string name, string parameters)
{
	map<string, CanLibLoader *>::iterator itcomponent;
	map<string, CCanAccess *>::iterator it;
	CanLibLoader *dlcan = 0;
	string nameComponent;

	it = m_ScanManagers.find(name);
	if (!(it == m_ScanManagers.end())) {
		return (*it).second;
	}

	nameComponent = name.substr(0, name.find_first_of(':'));
	itcomponent = m_Component.find(nameComponent);

	if (!(itcomponent == m_Component.end())) {
		dlcan = (*itcomponent).second;
	} else {
		dlcan = CanLibLoader::createInstance((char *)nameComponent.c_str());
		if (dlcan != 0)
			m_Component.insert(map<string, CanLibLoader *>::value_type(nameComponent, dlcan));
		else return 0;
	}
	CCanAccess *tcca = dlcan->openCanBus(name, parameters);
	return tcca;
}
}
