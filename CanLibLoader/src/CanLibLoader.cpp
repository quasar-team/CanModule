#include "CanLibLoader.h"
#include "LogIt.h"
#include <string>
#include <stdexcept>
#ifdef _WIN32
#include "CanLibLoaderWin.h"
#else
#include "CanLibLoaderLin.h"
#endif

using namespace std;
namespace CanModule
{
	CanLibLoader::CanLibLoader(const std::string& libName)
		//: m_canAccessInstance(0)
	{}

	CanLibLoader::~CanLibLoader() {}

	CanLibLoader* CanLibLoader::createInstance(const std::string& libName)
	{
		CanLibLoader* ret;
#ifdef WIN32
		ret = new CanLibLoaderWin(libName);
#else
		ret = new CanLibLoaderLin(libName);
#endif

		return ret;
	}

	void CanLibLoader::closeCanBus(CCanAccess *cInter)
	{
		LOG(Log::DBG) << "Canbus name to be deleted: " << cInter->getBusName();
		//	m_openCanAccessMap.erase(cInter->getBusName().c_str());

		delete cInter;
	}

	CCanAccess* CanLibLoader::openCanBus(string name, string parameters)
	{
		LOG(Log::DBG) << "Creating CCanAccess: name= " << name << " parameters= " << parameters;
		CCanAccess *tcca = createCanAccess();
		//The Logit instance of the executable is handled to the DLL at this point, so the instance is shared.
		tcca->initialiseLogging(LogItInstance::getInstance());

		bool c = tcca->createBus(name, parameters);
		if (c) {
			LOG(Log::DBG) << "Adding CCanAccess to the map: " << name;
			return tcca;
		}
		else
		{
			LOG(Log::ERR) << "Problem opening canBus: " << name;
			throw std::runtime_error("Problem when opening canBus" );
		}
		//	}
		return 0;
	}
}
