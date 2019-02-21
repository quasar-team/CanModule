/**
 * canmodule_wrapper h: this is C++ code compiled with extern "C"
 * to give a C API to canmodule which is callable
 */

#ifndef CANMODULE_WRAPPER_INCLUDEGUARD
#define CANMODULE_WRAPPER_INCLUDEGUARD
// for CanModule
#include <boost/thread.hpp>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>

#include <CanBusAccess.h>  // the CanModule entry point
#include <CanMessage.h>

#if WIN32
#else
#include "/usr/include/linux/can.h"
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif






#if WIN32
#define CAN_MAX_DLEN 8
/**
 * struct can_frame - basic CAN frame structure
 * @can_id:  CAN ID of the frame and CAN_*_FLAG flags, see canid_t definition
 * @can_dlc: frame payload length in byte (0 .. 8) aka data length code
 *           N.B. the DLC field from ISO 11898-1 Chapter 8.4.2.3 has a 1:1
 *           mapping of the 'data length code' to the real payload length
 * @data:    CAN frame payload (up to 8 byte)
 */
typedef uint32_t canid_t;
struct can_frame {
	canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	uint8_t    can_dlc; /* frame payload length in byte (0 .. CAN_MAX_DLEN) */
	uint8_t    data[CAN_MAX_DLEN]; // __attribute__((aligned(8)));
};
#endif

// C->C++ wrapper

/**
 * mutex protected buffer for CanMessage reception:
 * the reception handler drops it here and releases the mutex
 * so that the blocking wait for new message can get released.
 */
typedef struct {
//	boost::mutex reception_mtx;
//	boost::condition_variable_any reception_cond;
	bool newArrived;
	CanMessage receivedMessageBuffer;
} RECEPTION_t;

/**
 * access to ports of (vendor) bridges, for send and receive.
 * Each connection is one CAN port for sending and receiving messages. Bridge modules have varying
 * number or CAN ports and PC ports, depending on the module/vendor type.
 * The object references CanLibLoader and CCanAccess are of course "per vendor"
 * and "per PC port", but in order to simplify lets keep a distinct reference for each
 * connection. Those two objects have each several references therefore (or: several pointers point
 * to the same object).
 */
typedef enum { systec, anagate, peak } VENDORLIBS_t;
typedef struct {
	VENDORLIBS_t lib;
	CanModule::CanLibLoader *loader;
	bool isConnected;
	CanModule::CCanAccess *cca;
	RECEPTION_t reception;
	unsigned int delay ; // = 1000000; // 1kHz, default, works for iseg systec and anagate
} CAN_CONNECTION_t;

typedef void* canmodule_access_t;
EXTERNC CanMessage canmodule_reformat2CanModuleMessage( can_frame );
EXTERNC can_frame canmodule_reformat2LinuxFrame( CanMessage m_in );

EXTERNC void canmodule_init( int connectionIndex, Log::LOG_LEVEL loglevel, vector<string> canmoduleParameters );
EXTERNC bool canmodule_send( int connectionIndex, CanMessage cm ); // OK=0
EXTERNC CanMessage canmodule_waitForNewCanMessage( int connectionIndex );
EXTERNC void canmodule_testSignalSlot( int connectionIndex );
EXTERNC void canmodule_delete( int connectionIndex );
EXTERNC void canmodule_setDelay_us( int connectionIndex, unsigned int dd );



#undef EXTERNC

#endif // CANMODULE_WRAPPER_INCLUDEGUARD
