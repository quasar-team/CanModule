/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CCanAccess.h
 *
 *  Created on: Apr 4, 2011
 *      original author: vfilimon
 *      maintaining: mludwig, quasar team
 *
 *  This file is part of Quasar.
 *
 *  Quasar is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public Licence as published by
 *  the Free Software Foundation, either version 3 of the Licence.
 *
 *  Quasar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public Licence for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Quasar.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CCANACCESS_H_
#define CCANACCESS_H_

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <condition_variable>
#include <optional>

#include <boost/bind/bind.hpp>
#include <boost/signals2.hpp>

#include <LogIt.h>

#include <CanMessage.h>
#include <CanStatistics.h>
#include <CanModuleUtils.h>

#include "VERSION.h"



/*
 * CCanAccess is an abstract class that defines the interface for controlling a canbus. Different implementations for different hardware and platforms should
 * inherit this class and implement the pure virtual methods.
 */
namespace CanModule
{
/**
 * one (singleton) global signal for errors independent of bus. This is needed to listen to bus opening/creation errors where the bus does not yet exist.
 * All buses and errors go into this signal.
 *
 * The recommendation is:
 *    connect a handler to the global signal
 *    open the bus
 *    if successful, connect bus specific handlers for errors, receptions, port status changes
 *    if not successful, keep global handler and try again (server logic)
 *    of course you can keep the global handler connected as well, but you might also disconnect it (let the sigleton object go out of scope)
 *
 * boost::signal2 need implicit this-> pointers to work, that means they only work as methods of existing objects. So we need a class
 * for specifically implementing the globalErrorSignal, which is independent of the existence of lib and bus instances.
 */
class GlobalErrorSignaler /* singleton */
{

private:
	GlobalErrorSignaler(){};
	~GlobalErrorSignaler();

	static GlobalErrorSignaler *instancePtr;
	static LogItInstance *m_st_logIt;
	static Log::LogComponentHandle m_st_lh;
	boost::signals2::signal<void (const int,const char *,timeval &) > globalErrorSignal;

public:
	SHARED_LIB_EXPORT_DEFN static GlobalErrorSignaler* getInstance();
	SHARED_LIB_EXPORT_DEFN bool connectHandler( void (*fcnPtr)( int, const char*, timeval  ) );
	SHARED_LIB_EXPORT_DEFN bool disconnectHandler( void (*fcnPtr)( int, const char*, timeval  ) );
	SHARED_LIB_EXPORT_DEFN void disconnectAllHandlers();
	SHARED_LIB_EXPORT_DEFN void fireSignal( const int code, const char *msg );

	static void initializeLogIt(LogItInstance *remoteInstance);
};


const std::string LogItComponentName = "CanModule";
#define MLOG(LEVEL,THIS) LOG(Log::LEVEL) << __FUNCTION__ << " " << CanModule::LogItComponentName << " bus= " << THIS->getBusName() << " "

inline static void ms_sleep( int ms ){
	std::chrono::milliseconds delay( ms );
	std::this_thread::sleep_for( delay );
}

/**
 * implementation specific counter (high nibble of status bitpattern)
 * * 0x1<<28 = sock (linux)
 */
#define CANMODULE_STATUS_BP_SOCK (0x1<<28)
/**
 * implementation specific counter (high nibble of status bitpattern)
 * * 0x2<<28 = anagate (linux, windows)
 */
#define CANMODULE_STATUS_BP_ANAGATE (0x2<<28)
/**
 * implementation specific counter (high nibble of status bitpattern)
 * * 0x3<<28 = peak (windows)
 */
#define CANMODULE_STATUS_BP_PEAK (0x3<<28)
/**
 * implementation specific counter (high nibble of status bitpattern)
 * * 0x4<<28 = systec (windows)
 */
#define CANMODULE_STATUS_BP_SYSTEC (0x4<<28)


/**
 * returns a version string which is created at build-time, stemming from the CMakeLists.txt
 */
static std::string version(){ return( CanModule_VERSION ); }

/**
 * if a reconnect condition becomes true, a reconnect action is performed.
 * @param sendFail (default):
 * 		the number of allowed fails for a send attempt can be set (default=10).
 *
 * @param timeoutOnReception:
 * 		the number of seconds during which no message was received can be set (default=120sec). If the timeout is exceeded
 * 		the condition becomes true. For anagate, this condition is only checked before a sending attempt. For all others the condition
 * 		is checked in the supervisory thread.
 *
 * 	@param never:
 * 		this condition is never true, use it to switch off reconnection behavior. You might need this if you share a bridge between
 * 		several tasks.
 */
SHARED_LIB_EXPORT_DEFN enum class ReconnectAutoCondition { sendFail=0, timeoutOnReception, never };


/**
 * a reconnect action is performed if it is triggered by a reconnect condition of that channel/bus.
 * @param singleBus (default):
 * 		only the bus in question (this object) is reconnected, including it's reply handler, using the available implementation and vendor API.
 *
 * 	@param 	allBusesOnBridge:
 * 		all the buses/channels on the bridge where this channel/bus physically belongs to are reconnected. Only available for anagate, where
 * 		the whole bridge can be reset using it's ip number.
 *
 * 	@param hardReset:
 * 		the whole bridge receives some kind of "hard reset via software", only available for anagate (firmware reboot).
 */
SHARED_LIB_EXPORT_DEFN enum class ReconnectAction { singleBus=0, allBusesOnBridge };

struct CanParameters {
	long m_lBaudRate;
	unsigned int m_iOperationMode;
	unsigned int m_iTermination;
	unsigned int m_iHighSpeed;
	unsigned int m_iTimeStamp;
	unsigned int m_iSyncMode;
	unsigned int m_iTimeout;

	int	m_iNumberOfDetectedParameters;
	bool m_dontReconfigure;
	CanParameters() : m_lBaudRate(0), m_iOperationMode(0), m_iTermination(0),
			m_iHighSpeed(1), m_iTimeStamp(0),
			m_iSyncMode(0), m_iTimeout(6000),
			m_iNumberOfDetectedParameters(), m_dontReconfigure(false) {}

	void scanParameters(std::string parameters);
};

/** beta.v6 "an2" CANGetCounters anagate
The AnaGate CAN F series models can provide all nine TCP, CAN and error counters
from the web interface. A detailed description of the counters is available in the
device status chapter of the AnaGate CAN F series manual.
 */
SHARED_LIB_EXPORT_DEFN struct PORT_COUNTERS_t {
	unsigned int countTCPRx; // TCP Received counter.
	unsigned int countTCPTx; // TCP Transmitted counter.
	unsigned int countCANRx; // CAN Received counter.
	unsigned int countCANTx; // CAN Transmitted counter.
	unsigned int countCANRxErr; // CAN Bus Receive Error counter.
	unsigned int countCANTxErr; // CAN Bus Transmit Error counter.
	unsigned int countCANRxDisc; // CAN Discarded Rx Full Queue counter.
	unsigned int countCANTxDisc; // CAN Discarded Tx Full Queue counter.
	unsigned int countCANTimeout; // CAN Transmit Timeout counter.
};


/**
 * a (hardware) logging item generally has a message and a timestamp. Keep it simple.
 *
 * a log entry history of one port has many log items. anagate2 hw limits this to 100 but there is no reason to limit the vector size here
 * we can in fact keep a looooong history. we will just limit the max history to a reasonable value to avoid a coded mem hole.
 * Lets say portLog_max = 5000.
 * we drop any buffer overflow.
 *
 */
SHARED_LIB_EXPORT_DEFN struct  PORT_LOG_ITEM_t {
	std::string message;
	std::string timestamp;
};

SHARED_LIB_EXPORT_DEFN struct HARDWARE_DIAG_t {
	float temperature; // in deg C
	int uptime;      // in seconds
	unsigned int clientCount; // connected clients for this IP/module/submodule
	std::vector<std::string> clientIps; // decoded into strings, from unsigned int
	std::vector<unsigned int> clientPorts; // array of client ports
	std::vector<std::string> clientConnectionTimestamps; // time of initial client connection
};


class CCanAccess
{

public: // accessible API
	CCanAccess():
		m_sBusName(""),
		m_sendThrottleDelay( 0 ),
		m_reconnectCondition( CanModule::ReconnectAutoCondition::sendFail ),
		m_reconnectAction( CanModule::ReconnectAction::singleBus ),
		m_timeoutOnReception( 120 ),
		m_failedSendCountdown( 10 ),
		m_maxFailedSendCount( 10 ),
		m_lh(0),
		m_logItRemoteInstance( NULL ),
		m_portStatus( 0 ),
		m_canPortStateChanged_nbSlots_current( 0 ),
		m_canPortStateChanged_nbSlots_previous( 0 )

{
		resetTimeoutOnReception();
		resetTimeNow();
		CanModule::version();
};

	virtual ~CCanAccess() {};

	/**
	 * Method that sends a remote request trough the can bus channel. If the method createBus was not called before this, sendMessage will fail, as there is no
	 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
	 * @param cobID: Identifier that will be used for the request.
	 * @return Was the sent request successful ?
	 */
	virtual bool sendRemoteRequest(uint32_t cobID) = 0;

	/**
	 * Method that initialises a can bus channel. The following methods called upon the same object will be using this initialised channel.
	 *
	 * @param name: Name of the can bus channel. The specific mapping will change depending on the interface used. For example, accessing channel 0 for the
	 * 				systec interface would be using name "st:9", while in socket can the same channel would be "sock:can0".
	 * 				anagate interface would be an:192.168.1.2 for the default ip address
	 * @param parameters: Different parameters used for the initialisation. For using the parameters already in the hardware just set this to "Unspecified".
	 * 				anagate: pass the ip number
	 *
	 * @return and integer
	 * 0 = init OK, bus was created = OK
	 * 1 = init OK, bus was skipped because it exists already
	 * -1 = init failed
	 */
	virtual int createBus(const std::string name, const std::string parameters) = 0;
	virtual int createBus(const std::string name, const std::string parameters, float factor ) = 0;

	/**
	 * Method that sends a message through the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
	 * can bus channel to send a message through. SendMessage is non blocking and reentrant - as far as the various vendor APIs permit this.
	 * @param cobID: Identifier that will be used for the message.
	 * @param len: Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
	 * @param message: Message to be sent through the can bus.
	 * @param rtr: activate the Remote Transmission Request flag. Having this flag in a message with data/len is not part of the CAN standard,
	 * but since some hardware uses it anyway, the option to use it is provided.
	 * @param eff: indicate if we should use extended id for the message CAN 2.0B
	 * @return: Was the message sent successfully? If not, we may reconnect.
	 */
	virtual bool sendMessage(uint32_t cobID, unsigned char len, unsigned char *message, bool rtr = false, bool eff = false) = 0;
	bool sendMessage(CanMessage *canm)
	{
		if (!canm->c_eff && ( canm->c_id < 0 || canm->c_id > 2047 )){
			LOG(Log::WRN, m_lh) << __FUNCTION__ << " CAN ID= 0x"
					<< std::hex << canm->c_id
					<< " outside 11 bit (standard) range detected. Dropping message: Extended ID detected, but EFF flag false.";
			return false;
		}
		return sendMessage(canm->c_id, canm->c_dlc, canm->c_data, canm->c_rtr, canm->c_eff);
	}


	/**
	 * UNIFIED PORT status
	 *
	 * according to vendor and OS, acquire bus (port) status, and return one uint32_t bitpattern which has
	 * the same rules for all vendors. In fact the status for vendors is too different to be abstracted
	 * into a common bitpattern. Actually, talk to the HW, so use with parcimony while comms are ongoing:
	 * we do not really know how well the vendors have implemented that.
	 *
	 *
	 * the **implementation** occupies the highest nibble, and it is a counter (see CANMODULE_STATUS_BP_SOCK etc)
	 * * 0x1<<28 = sock (linux)
	 * * 0x2<<28 = anagate (linux, windows)
	 * * 0x3<<28 = peak (windows)
	 * * 0x4<<28 = systec (windows)
	 * * 0x5<<28....0xf<<28 = unused, for future use
	 *
	 *
	 * the **specific_status** occupies bits b0..b27, and it is a (composed) implementation specific bitpattern
	 *
	 * @param sock (linux): [ see can_netlink.h  enum can_state ]
	 * * b0: 0x1 = CAN_STATE_ERROR_ACTIVE   : RX/TX error count < 96
	 * * b1: 0x2 = CAN_STATE_ERROR_WARNING  : RX/TX error count < 128
	 * * b2: 0x4 = CAN_STATE_ERROR_PASSIVE  : RX/TX error count < 256
	 * * b3: 0x8 = CAN_STATE_BUS_OFF        : RX/TX error count >= 256
	 * * b4: 0x10 = CAN_STATE_STOPPED       : Device is stopped
	 * * b5: 0x20 = CAN_STATE_SLEEPING      : Device is sleeping
	 * * b6...b27 unused
	 *
	 * @param anagate (linux, windows): [ see CANDeviceConnectState ]
	 * CANCanDeviceConnectState , translate from counter (0 does not exists, thank you anagate)
	 * * 1 = DISCONNECTED   :
	 * * 2 = CONNECTING :
	 * * 3 = CONNECTED
	 * * 4 = DISCONNECTING
	 * * 5 = NOT_INITIALIZED
	 * * b3...b27: unused
	 * * I translate this into a simple bitpattern which is a counter :
	 * 000(does not occur), 001, 010, 011, 100, 101. Actually 011 means OK therefore. great.
	 *
	 * @param peak (windows): see PCANBasic.h:113
	 * * #define PCAN_ERROR_OK                0x00000U  // No error
	 * * #define PCAN_ERROR_XMTFULL           0x00001U  // Transmit buffer in CAN controller is full
	 * * #define PCAN_ERROR_OVERRUN           0x00002U  // CAN controller was read too late
	 * * #define PCAN_ERROR_BUSLIGHT          0x00004U  // Bus error: an error counter reached the 'light' limit
	 * * #define PCAN_ERROR_BUSHEAVY          0x00008U  // Bus error: an error counter reached the 'heavy' limit
	 * * #define PCAN_ERROR_BUSWARNING        PCAN_ERROR_BUSHEAVY // Bus error: an error counter reached the 'warning' limit
	 * * #define PCAN_ERROR_BUSPASSIVE        0x40000U  // Bus error: the CAN controller is error passive
	 * * #define PCAN_ERROR_BUSOFF            0x00010U  // Bus error: the CAN controller is in bus-off state
	 * * #define PCAN_ERROR_ANYBUSERR         (PCAN_ERROR_BUSWARNING | PCAN_ERROR_BUSLIGHT | PCAN_ERROR_BUSHEAVY | PCAN_ERROR_BUSOFF | PCAN_ERROR_BUSPASSIVE) // Mask for all bus errors
	 * * #define PCAN_ERROR_QRCVEMPTY         0x00020U  // Receive queue is empty
	 * * #define PCAN_ERROR_QOVERRUN          0x00040U  // Receive queue was read too late
	 * * #define PCAN_ERROR_QXMTFULL          0x00080U  // Transmit queue is full
	 * * #define PCAN_ERROR_REGTEST           0x00100U  // Test of the CAN controller hardware registers failed (no hardware found)
	 * * #define PCAN_ERROR_NODRIVER          0x00200U  // Driver not loaded
	 * * #define PCAN_ERROR_HWINUSE           0x00400U  // Hardware already in use by a Net
	 * * #define PCAN_ERROR_NETINUSE          0x00800U  // A Client is already connected to the Net
	 * * #define PCAN_ERROR_ILLHW             0x01400U  // Hardware handle is invalid
	 * * #define PCAN_ERROR_ILLNET            0x01800U  // Net handle is invalid
	 * * #define PCAN_ERROR_ILLCLIENT         0x01C00U  // Client handle is invalid
	 * * #define PCAN_ERROR_ILLHANDLE         (PCAN_ERROR_ILLHW | PCAN_ERROR_ILLNET | PCAN_ERROR_ILLCLIENT)  // Mask for all handle errors
	 * * #define PCAN_ERROR_RESOURCE          0x02000U  // Resource (FIFO, Client, timeout) cannot be created
	 * * #define PCAN_ERROR_ILLPARAMTYPE      0x04000U  // Invalid parameter
	 * * #define PCAN_ERROR_ILLPARAMVAL       0x08000U  // Invalid parameter value
	 * * #define PCAN_ERROR_UNKNOWN           0x10000U  // Unknown error
	 * * #define PCAN_ERROR_ILLDATA           0x20000U  // Invalid data, function, or action
	 * * #define PCAN_ERROR_CAUTION           0x2000000U  // An operation was successfully carried out, however, irregularities were registered
	 * * #define PCAN_ERROR_INITIALIZE        0x4000000U  // Channel is not initialized [Value was changed from 0x40000 to 0x4000000]
	 * * #define PCAN_ERROR_ILLOPERATION      0x8000000U  // Invalid operation [Value was changed from 0x80000 to 0x8000000]
	 *
	 * @param systec (windows): [ see UcanGetStatus  table19 (for CAN) and table20 (for USB) ]. This is a combination of socketcan bits and usb bits
	 * * b0: 0x1: tx overrun
	 * * b1: 0x2: rx overrun
	 * * b2: 0x4: error limit1 exceeded: warning limit
	 * * b3: 0x8: error limit2 exceeded: error passive
	 * * b4: 0x10: can controller is off
	 * * b5: unused
	 * * b6: 0x40: rx buffer overrun
	 * * b7: 0x80: tx buffer overrun
	 * * b8..b9: unused
	 * * b10: 0x400: transmit timeout, message dropped
	 * * b12..b11: unused
	 * * b13: 0x2000: module/usb got reset because of polling failure per second
	 * * b14: 0x4000: module/usb got reset because watchdog was not triggered
	 * * b15...b27: unused
	 *
	 * examples:
	 * * 0x3000.0000.000.0040 means "peak" implementation (peak@windows) receive queue was read too late
	 * * 0x1000.0000.000.0002 means "sock" implementation (peak or systec @linux) CAN_STATE_ERROR_WARNING
	 */
	virtual uint32_t getPortStatus() = 0;

	/**
	 * returns the bitrate of that port [bits/sec] according to what CanModule buffers say. This is
	 * the setting used for setting up the hardware, after any default rules have been applied, but
	 * BEFORE any vendor specific encoding into obscure bitpatterns occurs. This happens at port opening
	 * and the bitrate can only be changed at that moment. So please call this method just after you have
	 * opened the port. But also since there is no hw interaction and it just returns a buffer, you may
	 * call it as often as you like.
	 */
	virtual uint32_t getPortBitrate() = 0;

	/**
	 *  Returns the CanStatistics object, which contains bus statistics, bus load, counters and time stamps
	 */
	virtual void getStatistics( CanStatistics & result ) = 0;

	/**
	 * configuring the reconnection behavior: a condition triggers an action. The implementation is implementation-specific
	 * because not all vendor APIs permit the same behavior: not all combinations are available for all vendors/implementations.
	 * The implemented reconnection behavior is standardized nevertheless: "you get what you see". Foe each CanBus you have to choose
	 * one condition and one action. Condition detection and action execution are then automatic inside CanModule.
	 *
	 * @param CanModule::ReconnectAutoCondition cond
	 * 		* sendFail (default):
	 * 		  	a reconnection is triggered automatically if a send on the canBus fails. The number of failed sends can be set
	 *	 		(default is 10). The failed sending must be strictly consecutive, if there is a succeeded sending,
	 *  	    the internal counting restarts.
	 *
	 * 		* timeoutOnReception:
	 * 			the last time of a successful reception on a given channel can be monitored. If the last reception is
	 * 			older than 120 sec a reset is triggered automatically. This last successful reception time
	 * 			is calculated each time a send is invoked. The timeout is specified in seconds, default is 120sec.
	 *
	 * 		* never:
	 * 			this condition is always false and therefore no automatic reconnection action will be triggered. You can still
	 * 			call the explicit method reconnect( <action> ) but it is up to the client (server) to decide when.
	 *
	 * @param CanModule::ReconnectAction action
	 * 		* singleBus (default):
	 * 			a single connection (canbus, channel) is reset and reception handler is reconnected.
	 *
	 * 		* wholeBridge
	 * 			if available, the whole bridge is reset, affecting all physical channels on that bridge. Only available for anagate.
	 * 			If a bridge is shared between multiple tasks, all channels across tasks are reset, affecting all tasks connected to
	 * 			that bridge. Evidently, a given channel must only be used by at most one task.
	 * 				 */

	void setReconnectBehavior( CanModule::ReconnectAutoCondition cond, CanModule::ReconnectAction action ){
		m_reconnectCondition = cond;
		m_reconnectAction = action;
	};
	void setReconnectReceptionTimeout( unsigned int timeout ){ 	m_timeoutOnReception = timeout;	};
	void setReconnectFailedSendCount( unsigned int c ){ m_maxFailedSendCount = m_failedSendCountdown = c;	}
	CanModule::ReconnectAutoCondition getReconnectCondition() { return m_reconnectCondition; };
	CanModule::ReconnectAction getReconnectAction() { return m_reconnectAction; };


	/**
	 *  stop the bus.
	 */
	virtual void stopBus() = 0;

	/** Generally: do whatever shenanigans you need on the vendor API to get an idea of what the port is doing.
	 * then fill in the portState accordingly, as close as possible to the semantics of the enum.
	 * then send the port status to the canPortStateChanged signal
	 *
	 * We always have the CAN message error flags and their decoding to save the day, on the canMessageError signal
	 * And we can invoke directly the getPortStatus() as well to get the unified bitpattern.
	 */
	virtual void fetchAndPublishCanPortState () = 0;

	/*
	 * Signal that gets called when a can Message is received from the initialised can bus.
	 * In order to process this message manually, a handler needs to be connected to the signal.
	 *
	 * Example: myCCanAccessPointer->canMessageCame.connect(&myMessageRecievedHandler);
	 */
	boost::signals2::signal<void (const CanMessage &) > canMessageCame;


	/**
	 * Signal that gets called when a can Error is detected in the message
	 * In order to process this message manually, a handler needs to be connected to the signal.
	 *
	 * Example: myCCanAccessPointer->canMessageError.connect(&myErrorRecievedHandler);
	 */
	boost::signals2::signal<void (const int, const char *, timeval &) > canMessageError;

	/**
	 * signal gets called whenever there is a can port status change, across all vendors
	 * the int is the  enum CanModule::CanModuleUtils::CanModule_bus_state
	 * In order to process this message, a handler needs to be connected to the signal.
	 *
	 * Example: myCCanAccessPointer->canPortStateChanged.connect(&myPortSateChangedRecievedHandler);
	 */
	boost::signals2::signal<void (const int, const char *, timeval &) > canPortStateChanged;

	/**
	 * advanced diagnostics from the hardware. They can only be implemented for some implementations,
	 * but all implementations must have a dummy
	 */
	virtual std::vector<CanModule::PORT_LOG_ITEM_t> getHwLogMessages ( unsigned int n ) = 0;
	virtual std::optional<CanModule::HARDWARE_DIAG_t> getHwDiagnostics () = 0;
	virtual std::optional<CanModule::PORT_COUNTERS_t> getHwCounters () = 0;

	std::string& getBusName();
	bool initialiseLogging(LogItInstance* remoteInstance);
	LogItInstance* getLogItInstance();
	std::vector<std::string> parseNameAndParameters( std::string name, std::string parameters);
	void triggerReconnectionThread();
	void waitForReconnectionThreadTrigger();
	void decreaseSendFailedCountdown();
	void resetSendFailedCountdown(){ m_failedSendCountdown = m_maxFailedSendCount; 	}

	static std::string reconnectConditionString(CanModule::ReconnectAutoCondition c);
	static std::string reconnectActionString(CanModule::ReconnectAction c);

	// convenience
	float getFramerateDelay(){ return m_sendThrottleDelay; }

protected: // not public, but inherited
	std::string m_sBusName;
	CanParameters m_CanParameters;
	int m_sendThrottleDelay; // in us.

	// reconnection, reconnection thread triggering
    CanModule::ReconnectAutoCondition m_reconnectCondition;
    CanModule::ReconnectAction m_reconnectAction;
	std::atomic_uint m_timeoutOnReception;
	std::atomic_int m_failedSendCountdown;
	std::atomic_uint m_maxFailedSendCount;
	std::thread *m_hCanReconnectionThread;     // ptr thread, it's a private method of the class (virtual) // unused for peak
	std::atomic_bool m_reconnectTrigger;            // trigger stuff: predicate of the condition var
	std::mutex m_reconnection_mtx;             // trigger stuff
	std::condition_variable m_reconnection_cv; // trigger stuff

	bool hasTimeoutOnReception();
	void resetTimeoutOnReception() { m_dreceived = std::chrono::high_resolution_clock::now(); } // reset the internal reconnection timeout counter
	void resetTimeNow() { m_dnow = std::chrono::high_resolution_clock::now(); }
	void publishPortStatusChanged ( unsigned int status );
	void us_sleep( int us );

private: // object scope
	static LogItInstance* st_logItRemoteInstance;
	Log::LogComponentHandle m_lh;
	LogItInstance* m_logItRemoteInstance;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_dnow, m_dreceived, m_dtransmitted, m_dopen;
	unsigned int m_portStatus;
	// keep track of connected handlers. If the number changes, send a signal so that all handlers are updated
	unsigned int m_canPortStateChanged_nbSlots_current;
	unsigned int m_canPortStateChanged_nbSlots_previous;


};
}; // namespace CanModule
#endif /* CCANACCESS_H_ */
