#include "CanLibExternCWrapper.h"
#include "CanLibLoader.h"
#include "CCanAccess.h"
#include <map>
#include "LogIt.h"
#include "MessageSharedQueue.h"
#include "boost/thread.hpp"

using namespace CanModule;

CanLibLoader* canLibInstance;
std::map<char*, CCanAccess *> openCanAccessMap;

//If the wrapper is not initialised the rest of functions will throw an exception, as they can't work without previously loading a lib
bool isWrapperInitialised = false;

typedef MessageSharedQueue<CanMsgStruct> MessageQueue;

struct MessageQueueAccess
{
	MessageQueue* m_messageQueue;
	char* m_canBusName;
	void initialize(char* id)
	{
		m_messageQueue = new MessageQueue();
		m_canBusName = new char[strlen(id) + 1];
		strncpy(m_canBusName, id, strlen(id) + 1);
	}
	void operator()(const CanMsgStruct/*&*/ message) const
	{
		m_messageQueue->put(message);
		LOG(Log::DBG) << "New message inserted into queue: m_canBusName[" << m_canBusName << "]; c_id [" << message.c_id << "]";
	}
};

std::map<char*, MessageQueueAccess *> queueAccessMap;

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
		//throw std::runtime_error("Error: The functionality of the Wrapper is being accesed without having loaded a library.");
	}
}

extern "C" void openCanBus(char* canBusName, char* parameters)
{
	checkIfWrapperIsInitialised();
	std::map<char*, CCanAccess *>::iterator it;
	it = openCanAccessMap.find(canBusName);
	if (it == openCanAccessMap.end())
	{
		CCanAccess* canBusAccessInstance = canLibInstance->openCanBus(canBusName, parameters);
		openCanAccessMap.insert(std::map<char*, CCanAccess*>::value_type(canBusName, canBusAccessInstance));
		MessageQueueAccess* queueAccess = new MessageQueueAccess();
		queueAccess->initialize(canBusName);

		queueAccessMap.insert(std::map<char*, MessageQueueAccess*>::value_type(canBusName, queueAccess));
		canBusAccessInstance->canMessageCame.connect(*queueAccess);

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
	bool success = false;
	if (it == openCanAccessMap.end())
	{
		LOG(Log::ERR) << "Error: this port is not opened";
	}
	else
	{
		canLibInstance->closeCanBus(it->second);
		success = true;
	}
	if(success)
	{
		openCanAccessMap.erase(canBusName);
		queueAccessMap.erase(canBusName);
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

extern "C"  bool sendMessageTimeout(char* canBusName, short cobID, unsigned char len, unsigned char *message, int timeout, bool rtr)
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
		boost::thread* thread;
		thread = new boost::thread(boost::bind(&CCanAccess::sendMessage, it->second, cobID, len, message, rtr));
		return thread->try_join_for(boost::chrono::milliseconds(timeout));
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
		(it->second)->canMessageCame.disconnect_all_slots();
		(it->second)->canMessageCame.connect(handler);		
		queueAccessMap.erase(canBusName);
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

MessageQueueAccess* extractMessageQueue(char* canBusName)
{
	std::map<char*, MessageQueueAccess *>::iterator it;
	it = queueAccessMap.find(canBusName);
	if (it == queueAccessMap.end())
	{
		LOG(Log::ERR) << "Error: requested non existent queue";
		return NULL;
	}
	else
	{
		return it->second;
	}
}

extern "C" bool readMessage(char* canBusName, struct CanMsgStruct* msg)
{
	size_t size = 0;
	MessageQueue* msgQueue = extractMessageQueue(canBusName)->m_messageQueue;
	if (msgQueue->isEmpty())
	{
		return false;
	}
	CanMsgStruct messageFromQueue = msgQueue->blockingTake(size);
	msg->c_dlc = messageFromQueue.c_dlc;
	msg->c_ff = messageFromQueue.c_ff;
	msg->c_id = messageFromQueue.c_id;
	msg->c_rtr = messageFromQueue.c_rtr;
	msg->c_time = messageFromQueue.c_time;
	memcpy(msg->c_data, messageFromQueue.c_data, 8);
	return true;
}

extern "C" bool readSpecificMessage(char* canBusName, short cobID, struct CanMsgStruct* msg)
{
	size_t size = 0;
	MessageQueue* msgQueue = extractMessageQueue(canBusName)->m_messageQueue;
	CanMsgStruct messageFromQueue;
	bool foundMessage = false;
	if (msgQueue->isEmpty())
	{
		return false;
	}
	for (int i = 0; i < msgQueue->getSize(); i++)
	{
		messageFromQueue = msgQueue->blockingTake(size);
		if (messageFromQueue.c_id == cobID)
		{
			foundMessage = true;
			break;
		}
		else
		{			
			msgQueue->put(messageFromQueue);
		}
	}
	if (!foundMessage)
	{
		return false;
	}
	msg->c_dlc = messageFromQueue.c_dlc;
	msg->c_ff = messageFromQueue.c_ff;
	msg->c_id = messageFromQueue.c_id;
	msg->c_rtr = messageFromQueue.c_rtr;
	msg->c_time = messageFromQueue.c_time;
	memcpy(msg->c_data, messageFromQueue.c_data, 8);
	return true;
}

extern "C" bool readSpecificMessageSkip(char* canBusName, short cobID, struct CanMsgStruct* msg)
{
	size_t size = 0;
	MessageQueue* msgQueue = extractMessageQueue(canBusName)->m_messageQueue;
	CanMsgStruct messageFromQueue;
	bool foundMessage = false;
	if (msgQueue->isEmpty())
	{
		return false;
	}
	for (int i = 0; i < msgQueue->getSize(); i++)
	{
		messageFromQueue = msgQueue->blockingTake(size);
		if (messageFromQueue.c_id == cobID)
		{
			foundMessage = true;
			break;
		}
	}
	if (!foundMessage)
	{
		return false;
	}
	msg->c_dlc = messageFromQueue.c_dlc;
	msg->c_ff = messageFromQueue.c_ff;
	msg->c_id = messageFromQueue.c_id;
	msg->c_rtr = messageFromQueue.c_rtr;
	msg->c_time = messageFromQueue.c_time;
	memcpy(msg->c_data, messageFromQueue.c_data, 8);
	return true;
}

extern "C" bool readMessageTimeout(char* canBusName, int timeout, struct CanMsgStruct* msg)
{
	size_t size = 0;
	bool validItem = false;
	MessageQueue* msgQueue = extractMessageQueue(canBusName)->m_messageQueue;	
	CanMsgStruct messageFromQueue = msgQueue->maxWaitTake(validItem, size, timeout);
	if (!validItem)
	{
		return false;
	}
	msg->c_dlc = messageFromQueue.c_dlc;
	msg->c_ff = messageFromQueue.c_ff;
	msg->c_id = messageFromQueue.c_id;
	msg->c_rtr = messageFromQueue.c_rtr;
	msg->c_time = messageFromQueue.c_time;
	memcpy(msg->c_data, messageFromQueue.c_data, 8);
	return true;
}

extern "C" bool messageSync(char* canBusName, int timeout)
{
	size_t size = 0;
	MessageQueue* msgQueue = extractMessageQueue(canBusName)->m_messageQueue;
	return msgQueue->maxWaitForNonEmpty(timeout);
}


extern "C" bool messageSyncSpecific(char* canBusName, short cobID, int timeout)
{
	return false;
}
