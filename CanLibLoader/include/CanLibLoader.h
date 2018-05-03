#pragma once
#include <string>
#include <map>
#include "CCanAccess.h"
#include "ExportDefinition.h"

namespace CanModule 
{
	class CanLibLoader
	{
	protected:
		//Empty constructor
		CanLibLoader(const std::string& libName);
	public:
		//Will cleanup the loaded dynamic library
		virtual ~CanLibLoader();
	SHARED_LIB_EXPORT_DEFN static CanLibLoader* createInstance(const std::string& libName);

	SHARED_LIB_EXPORT_DEFN CanModule::CCanAccess * openCanBus(std::string name, std::string parameters);
		void closeCanBus(CanModule::CCanAccess *cca);
	protected:
		//Load a dynamic library.
		virtual void dynamicallyLoadLib(const std::string& libName) = 0;
		//Uses the loaded library to create a HAL object and store it in p_halInstance
		virtual CCanAccess* createCanAccess() = 0;
	private:
	};
}
