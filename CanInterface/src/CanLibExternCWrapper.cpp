#include "CanLibExternCWrapper.h"
#include "CanLibLoader.h"
#include "CCanAccess.h"
#include <map>
#include "LogIt.h"
using namespace CCC;
CanLibLoader* canLibInstance;
std::map<char*, CCanAccess *> openCanAccessMap;
//If the wrapper is not initialised the rest of functions will throw an exception, as they can't work without previously loading a lib
bool isWrapperInitialised = false;
extern "C" void initializeWrapper(char* dynamicLibraryName)
{
	canLibInstance = CanLibLoader::createInstance(dynamicLibraryName);
	isWrapperInitialised = true;
}
void checkIfWrapperIsInitialised()
{
	if(!isWrapperInitialised)
	{
		LOG(Log::ERR) << "Error: The functionality of the Wrapper is being accesed without having loaded a library. Maybe you forgot to call the function 'initializeWrapper'?. Program will now exit.";
		throw std::runtime_error("Error: The functionality of the Wrapper is being accesed without having loaded a library.");
	}
}

extern "C" void openCanBus(char* canBusName, char* parameters)
{
	checkIfWrapperIsInitialised();
	std::map<char*, CCanAccess *>::iterator it;
	it = openCanAccessMap.find(canBusName);
	if (it == openCanAccessMap.end())
	{
		openCanAccessMap.insert(std::map<char*, CCanAccess*>::value_type(canBusName, canLibInstance->openCanBus(canBusName, parameters)));
	}
	else
	{
		LOG(Log::ERR) << "Error: this port is already opened";
	}

}

extern "C" void closeCanBus(char* canBusName)
{
	checkIfWrapperIsInitialised();
	std::map<char*, CCanAccess *>::iterator it;
	it = openCanAccessMap.find(canBusName);
	if (it == openCanAccessMap.end())
	{
		LOG(Log::ERR) << "Error: this port is not opened";

	}
	else
	{
		canLibInstance->closeCanBus(it->second);
	}
}

/*
 * Method that sends a remote request trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
 * @param cobID: Identifier that will be used for the request.
 * @return: Was the initialisation process successful?
 */
extern "C" bool sendRemoteRequest(char* canBusName, short cobID)
{
	checkIfWrapperIsInitialised();
	std::map<char*, CCanAccess *>::iterator it;
	it = openCanAccessMap.find(canBusName);
	if (it == openCanAccessMap.end())
	{
		LOG(Log::ERR) << "Error: this port is not opened";
		return false;
	}
	else
	{
		return (it->second)->sendRemoteRequest(cobID);
	}
}

/*
 * Method that sends a message trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
 * can bus channel to send a message trough.
 * @param cobID: Identifier that will be used for the message.
 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
 * @param message: Message to be sent trough the can bus.
 * @return: Was the initialisation process successful?
 */
extern "C" bool sendMessage(char* canBusName, short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	checkIfWrapperIsInitialised();
	std::map<char*, CCanAccess *>::iterator it;
	it = openCanAccessMap.find(canBusName);
	if (it == openCanAccessMap.end())
	{
		LOG(Log::ERR) << "Error: this port is not opened";
		return false;
	}
	else
	{
		return (it->second)->sendMessage(cobID, len, message, rtr);
	}
}

/*
 * Signal that will be called when a can Message arrives into the initialised can bus.
 * In order to process this message manually, a handler needs to be connected to the signal.
 *
 * Example: myCCanAccessPointer->canMessageCame.connect(&myMessageRecievedHandler);
 */
//boost::signals2::signal<void (const CanMessage &) > canMessageCame;TODO: set handler to char* canBusName
extern "C" void connectMessageCameToHandler(char* canBusName, void (*handler) (const struct CanMsgStruct))
{
	checkIfWrapperIsInitialised();
	std::map<char*, CCanAccess *>::iterator it;
	it = openCanAccessMap.find(canBusName);
	if (it == openCanAccessMap.end())
	{
		LOG(Log::ERR) << "Error: this port is not opened";
	}
	else
	{
		(it->second)->canMessageCame.connect(handler);
	}
}
/*
 * Signal that will be called when a can Error arrives into the initialised can bus.
 * In order to process this message manually, a handler needs to be connected to the signal.
 *
 * Example: myCCanAccessPointer->canMessageError.connect(&myErrorRecievedHandler);
 */
//boost::signals2::signal<void (const int,const char *,timeval &) > canMessageError;
extern "C" void connectMessageErrorToHandler(char* canBusName, void(*handler) (const int, const char *, timeval /*&*/))
{
	checkIfWrapperIsInitialised();
	std::map<char*, CCanAccess *>::iterator it;
	it = openCanAccessMap.find(canBusName);
	if (it == openCanAccessMap.end())
	{
		LOG(Log::ERR) << "Error: this port is not opened";
	}
	else
	{
		(it->second)->canMessageError.connect(handler);
	}
}
