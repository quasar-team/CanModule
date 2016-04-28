#ifndef ___CANLIBEXTERN___
#define ___CANLIBEXTERN___
#include "CanMessage.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Method that initialises the Wrapper by loading the dynamic library (Which should contain the implementation of the CCC) given by the name passed as an attribute.
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
 * Method that sends a message trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
 * can bus channel to send a message trough.
 * @param canBusName: Name of the can bus channel form where the message will be sent. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
 * @param cobID: Identifier that will be used for the message.
 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
 * @param message: Message to be sent trough the can bus.
 * @return: Was the initialisation process successful?
 */
bool sendMessage(char* canBusName, short cobID, unsigned char len, unsigned char *message, bool rtr = false);

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
