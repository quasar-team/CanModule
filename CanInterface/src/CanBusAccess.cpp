
#include "CanBusAccess.h"

namespace CanModule
{
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
		}
		else {
			dlcan = CanLibLoader::createInstance((char *)nameComponent.c_str());
			if (dlcan != 0)
				m_Component.insert(map<string, CanLibLoader *>::value_type(nameComponent, dlcan));
			else return 0;
		}
		CCanAccess *tcca = dlcan->openCanBus(name, parameters);
		return tcca;
	}
}