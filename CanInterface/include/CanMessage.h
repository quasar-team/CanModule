/**
 * CanMessage.h
 *
 *  Created on: Nov 24, 2014
 *      Author: pnikiel
 */

#ifndef CANMESSAGE_H_
#define CANMESSAGE_H_

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/time.h>
#endif

/**
 * Struct that defines a Can Message, used by CCanAccess
 */
typedef struct CanMsgStruct
{
	//Identifier of the message
	long c_id;
	//Flags activated in the message
	unsigned char c_ff;
	//Length of the message
	unsigned char c_dlc;
	//Body of the message
	unsigned char c_data[8];
	//Timestamp of the message
	struct timeval	c_time;
	//Is the message a remote Transmission request? Must be true for remote request frames and false for data frames
	bool c_rtr;
	// Does this message use extended IDs
	bool c_eff;
#ifdef __cplusplus
public:
	CanMsgStruct() :
		c_id(0),
		c_ff(0),
		c_dlc(0),
		c_rtr(false),
		c_eff(false)
		{
			/// todo memset, and message length > 8 for extended CAN
			for (int i=0; i<8; i++)
				c_data[i] = 0;
		}
#endif //Be careful when using this struct from C

} CanMessage;

#endif /* CANMESSAGE_H_ */
