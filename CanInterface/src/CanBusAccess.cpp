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
/**
 * these static function pointers are needed to register a user handler
 * for the CanModule-C-Wrapper. And no - it is not possible to have a map
 * of pointers, otherwise the threads/mutexes will not work.
 * Why do we have 16 and not 8, since the systec16 has only 8 CAN ports per USB,
 * and no other module has more ? Simple answer: the ISEG HAL can handle up to
 * 16 receiver threads (hardcoded), so let's play is safe. It is a  bit unclear
 * what ISEG does with 8..15, but at least CanModule supports this. Most probably
 * the slots 8..15 are unused, though.
 */
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot0 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot1 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot2 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot3 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot4 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot5 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot6 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot7 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot8 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot9 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot10 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot11 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot12 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot13 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot14 = 0;
/* static */ boost::function< void ( const CanMsgStruct ) > CCanAccess::fw_slot15 = 0;

void CanBusAccess::closeCanBus(CCanAccess *cInter)
{
	CanLibLoader *dlcan;
	map<string, CanLibLoader *>::iterator itcomponent;
	string bus = cInter->getBusName();
	string nameComponent = bus.substr(0, bus.find_first_of(':'));

	itcomponent = m_Component.find(nameComponent);
	dlcan = (*itcomponent).second;
	dlcan->closeCanBus(cInter);
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
