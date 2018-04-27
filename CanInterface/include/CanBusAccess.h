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
