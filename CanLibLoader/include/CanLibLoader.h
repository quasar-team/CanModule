#pragma once
#include <string>
#include <map>
#include "CCanAccess.h"

class CanLibLoader
{
 protected:
	//Empty constructor
	CanLibLoader(const std::string& libName);
 public:	
	//Will cleanup the loaded dynamic library
	virtual ~CanLibLoader();
	static CanLibLoader* createInstance(const std::string& libName);

	CCC::CCanAccess * openCanBus(std::string name, std::string parameters);
	void closeCanBus(CCC::CCanAccess *cca);
 protected:
	//Load a dynamic library.
	 virtual void dynamicallyLoadLib(const std::string& libName)=0;
	//Uses the loaded library to create a HAL object and store it in p_halInstance
	 virtual void createCanAccess()=0;
	 CCC::CCanAccess *m_canAccessInstance;
 private:
	 bool isCanPortOpen(std::string pn) { return (m_openCanAccessMap.find(pn) != m_openCanAccessMap.end()); }
	 std::map<std::string, CCC::CCanAccess *> m_openCanAccessMap;
};
