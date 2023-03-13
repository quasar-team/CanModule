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

#include <boost/signals2.hpp>
/**
 * a global signal for errors independent of bus. This is needed to listen to bus opening/creation errors where the bus does not yet exist.
 * All buses and errors go into this signal.
 *
 * The recommendation is:
 *    connect a handler to the global signal
 *    open the bus
 *    if successful, connect bus specific handlers for errors, receptions, port status changes, then disconnect global handler
 *    if not successful, keep global handler and try again (server logic)
 */
//boost::signals2::signal<void (const int,const char *,timeval &) > globalError;


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
	CanBusAccess() : m_libLoader_map(), m_canAccess_map() {};
	CCanAccess * openCanBus(string name, string parameters);
	void closeCanBus(CCanAccess *cca);

private:
	bool isCanPortOpen(string pn) { return (m_canAccess_map.find(pn) != m_canAccess_map.end()); }
	map<string, CanLibLoader *> m_libLoader_map;
	map<string, CCanAccess *> m_canAccess_map;
};
}
#endif // CANBUSACCESS_H
