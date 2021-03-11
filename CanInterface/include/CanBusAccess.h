/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanBusAccess.h
 *
 *  Created on: Oct 22, 2014
 *      Author: Piotr Nikiel <piotr@nikiel.info>, quasar team
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

#ifndef CANBUSACCESS_H
#define CANBUSACCESS_H

#include "CanLibLoader.h"
#include "CCanAccess.h"
#include <map>
#include <string>

#pragma once

using namespace std;

namespace CanModule
{
	/**
	 * CanBusAccess class ensure a connection to can hardware.
	 * it can create the connection to different hardware at same time
	 * the syntax of the name is "name of component:name of the channel"
	 */
	class CanBusAccess {

	public:
		CanBusAccess() : m_Component(), m_ScanManagers() {};
		CCanAccess * openCanBus(string name, string parameters);
		void closeCanBus(CCanAccess *cca);

	private:
		bool isCanPortOpen(string pn) { return (m_ScanManagers.find(pn) != m_ScanManagers.end()); }
		map<string, CanLibLoader *> m_Component;
		map<string, CCanAccess *> m_ScanManagers;
	};
}
#endif // CANBUSACCESS_H
