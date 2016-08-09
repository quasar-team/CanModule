#ifndef ___CANLIBEXTERN___
#define ___CANLIBEXTERN___
#include "CanMessage.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Method that initialises the Wrapper by loading the dynamic library (Which should contain the implementation of the CanModule) given by the name passed as an attribute.
 * This method should be called before any of the other methods (Since they will use this library). Otherwise an exception will be thrown.
 */
void initializeWrapper(char* dynamicLibraryName);
/*
* Method that initialises a can bus channel. The following methods called will access this if the same name is referenced
* @param canBusName: Name of the can bus channel. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
* 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
* @param parameters: Different parameters used for the initialisation. For using the default parameters just set this to "Unspecified"
*/
void openCanBus(char* canBusName, char* parameters);

/*
* Closes a can bus with the given name
*/
void closeCanBus(char* canBusName);

/*
 * Method that sends a remote request trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
 * @param canBusName: Name of the can bus channel form where the request will be sent. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
 * @param cobID: Identifier that will be used for the request.
 * @return: Was the initialisation process successful?
 */
bool sendRemoteRequest(char* canBusName, short cobID);

/*
 * Method that reads a can message from the message queue. If a handler was specified using the method connectMessageCameToHandler, the messages will never arrive to the message queue.
 * @param: canBusName name of the channel from where the message is being read
 * @param: CanMsgStruct* If available, the message will be copied into this struct
 * @return: Returns false if the queue is empty and true otherwise
 */
bool readMessage(char* canBusName, struct CanMsgStruct*);

/*
* Method that reads a can message with an specific ID from the message queue. If a handler was specified using the method connectMessageCameToHandler, the messages will never arrive to the message queue.
* @param: canBusName name of the channel from where the message is being read
* @param: cobID identifier of the specific message the user wants to read
* @param: CanMsgStruct* If available, the message will be copied into this struct
* @return: Returns false if the queue is empty or there is no message with the specified id and true otherwise
*/
bool readSpecificMessage(char* canBusName, short cobID ,struct CanMsgStruct*);

/*
* Method that reads a can message with an specific ID from the message queue, skipping and discarding all the messages before that one. If a handler was specified using the method connectMessageCameToHandler, the messages will never arrive to the message queue.
* @param: canBusName name of the channel from where the message is being read
* @param: cobID identifier of the specific message the user wants to read
* @param: CanMsgStruct* If available, the message will be copied into this struct
* @return: Returns false if the queue is empty or there is no message with the specified id and true otherwise
*/
bool readSpecificMessageSkip(char* canBusName, short cobID, struct CanMsgStruct*);

/*
* Method that reads a can message from the message queue, and waits for it for a period of time if it is not available. If a handler was specified using the method connectMessageCameToHandler, the messages will never arrive to the message queue.
* Using this method, the message WONT be taken away from the queue or returned.
* @param: canBusName name of the channel from where the message is being read
* @param: timeout Time to wait for the message in milliseconds
* @param: CanMsgStruct* If available, the message will be copied into this struct
* @return: Returns false if the queue is empty and the timeout expired, true otherwise
*/
bool readMessageTimeout(char* canBusName, int timeout ,struct CanMsgStruct*);

/*
* Method that waits for a can message from the message queue, and waits for it for a period of time if it is not available. If a handler was specified using the method connectMessageCameToHandler, the messages will never arrive to the message queue.
* Using this method, the message WONT be taken away from the queue or returned.
* @param: canBusName name of the channel from where the message is being read
* @param: timeout Time to wait for the message in milliseconds
* @return: Returns false if the queue is empty and the timeout expired, true otherwise
*/
bool messageSync(char* canBusName, int timeout);

/*
* Method that waits for a can message with an specific ID from the message queue, and waits for it for a period of time if it is not available. If a handler was specified using the method connectMessageCameToHandler, the messages will never arrive to the message queue.
* Using this method, the message WONT be taken away from the queue or returned.
* @param: canBusName name of the channel from where the message is being read
* @param: cobID identifier of the specific message the user wants to read
* @param: timeout Time to wait for the message in milliseconds
* @return: Returns false if the queue is empty and the timeout expired, true otherwise
*/
bool messageSyncSpecific(char* canBusName, short cobID, int timeout);

/*
 * Method that sends a message trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
 * can bus channel to send a message trough.
 * @param canBusName: Name of the can bus channel form where the message will be sent. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
 * @param cobID: Identifier that will be used for the message.
 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
 * @param message: Message to be sent trough the can bus.
 * @param rtr: If this flag is set to true, the message will be marked as a signal message, even having a body. Signal messages don't have bodies on the CAN specification, but some hardware systems such as
 * WIENER use normal messages marked as signal as part of their protocol.
 * @return: Was the initialisation process successful?
 */
bool sendMessage(char* canBusName, short cobID, unsigned char len, unsigned char *message, bool rtr = false);

/*
* Method that sends a message trough the can bus channel. If the process takes more than the specified MS, the process will stop.
* If the method createBUS was not called before this, sendMessage will fail, as there is no can bus channel to send a message trough.
* @param canBusName: Name of the can bus channel form where the message will be sent. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
* 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
* @param cobID: Identifier that will be used for the message.
* @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
* @param message: Message to be sent trough the can bus.
* @param: timeout Time to wait for the message in milliseconds
* @param rtr: If this flag is set to true, the message will be marked as a signal message, even having a body. Signal messages don't have bodies on the CAN specification, but some hardware systems such as
* WIENER use normal messages marked as signal as part of their protocol.
* @return: Was the initialisation process successful?
*/
bool sendMessageTimeout(char* canBusName, short cobID, unsigned char len, unsigned char *message, int timeout, bool rtr = false);
/*
 * This method will connect a handler to a can bus module (identified by canBusName) which will be called once a can message arrives to this can bus module.
 * The module needs to be initialize before calling this method
 */
void connectMessageCameToHandler(char* canBusName, void (*handler) (const struct CanMsgStruct));
/*
* This method will connect a handler to a can bus module (identified by canBusName) which will be called once a can error arrives to this can bus module.
* The module needs to be initialize before calling this method
*/
void connectMessageErrorToHandler(char* canBusName, void(*handler) (const int, const char *, struct timeval /*&*/));

#ifdef __cplusplus
}
#endif
#endif// ___CANLIBEXTERN___
