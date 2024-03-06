/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * SockCanScan.cpp
 *
 *  Created on: Jul 21, 2011
 *  Based on work by vfilimon
 *  Rework and logging done by Piotr Nikiel <piotr@nikiel.info>
 *      mludwig at cern dot ch
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
 */
#include "SockCanScan.h"
#include "UdevAnalyserForPeak.h"

#include <thread>
#include <mutex>
#include <condition_variable>

#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <linux/sockios.h> // needed for SIOCGSTAMP in can-utils kernel since 4.feb.2020 3.10.0-1062.12.1.el7.x86_64

#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>

#include <CanModuleUtils.h>

#include <LogIt.h>

/* static */ std::map<std::string, std::string> CSockCanScan::m_busMap;
std::mutex sockReconnectMutex; // protect global m_busMap

#define MLOGSOCK(LEVEL,THIS) LOG(Log::LEVEL, THIS->logItHandle()) << __FUNCTION__ << " sock bus= " << THIS->getBusName() << " "

/**
 * This function creates an instance of this class and returns it.
 * It will be loaded trough the dll to access the rest of the code.
 */
extern "C" CCanAccess *getCanBusAccess()
{
	CCanAccess *cc = new CSockCanScan();
	return cc;
}

CSockCanScan::CSockCanScan() :
							m_CanScanThreadRunEnableFlag(false),
							m_sock(0),
							m_canMessageErrorCode(0),
							m_hCanScanThread( NULL ),
							m_busName("nobus"),
							m_logItHandleSock(0),
							m_gsig( NULL )
{
	m_statistics.setTimeSinceOpened();
	m_statistics.beginNewRun();
	m_failedSendCountdown = m_maxFailedSendCount;
}

/**
 * does not get called because advanced diag is not available for this implementation.
 * But we provide a proper return type nevertheless
 */
/* virtual */ std::vector<CanModule::PORT_LOG_ITEM_t> CSockCanScan::getHwLogMessages ( unsigned int n ){
	std::vector<CanModule::PORT_LOG_ITEM_t> log;
	return log;
}
/* virtual */  CanModule::HARDWARE_DIAG_t CSockCanScan::getHwDiagnostics (){
	CanModule::HARDWARE_DIAG_t d;
	return d;
}
/* virtual */ CanModule::PORT_COUNTERS_t CSockCanScan::getHwCounters (){
	CanModule::PORT_COUNTERS_t c;
	return c;
}
#if 0
/* virtual */ void CSockCanScan::setReconnectBehavior( CanModule::ReconnectAutoCondition cond, CanModule::ReconnectAction action ){
	m_reconnectCondition = cond;
	m_reconnectAction = action;
};
#endif
/**
 * (virtual) forced implementation. Generally: do whatever shenanigans you need on the vendor API and fill in the portState accordingly, stay
 * close to the semantics of the enum.
 *
 * sockcan specific implementation: obtain port status from the kernel, plus an extra LogIt message if we have an error
 */
/* virtual */ void CSockCanScan::fetchAndPublishCanPortState ()
{
	// we could also use the unified port status for this and strip off the implementation bits
	// but using can_get_state directly is less complex and has less dependencies


	int portState;
	if ( can_get_state(m_channelName.c_str(), &portState) < 0 )
	{
		// are we in DontConfigure mode? if so, it is okay for get_state to fail
		if ( m_CanParameters.m_dontReconfigure ){
			return;
		} else {
			portState = CanModule::CanModule_bus_state::CANMODULE_NOSTATE;
			// publishPortStatus( CanModule::CanModuleUtils::CanModule_bus_state::CANMODULE_NOSTATE, "Can't get state via netlink", true);
		}
	}
	// we LogIt the real errors
	switch ( portState ){
	case CanModule::CAN_STATE_ERROR_PASSIVE:
	case CanModule::CAN_STATE_BUS_OFF:
	case CanModule::CAN_STATE_STOPPED:
	case CanModule::CANMODULE_ERROR:
	{
		std::string msg = CanModule::translateCanBusStateToText((CanModule::CanModule_bus_state) portState );
		MLOGSOCK(ERR, this) << msg << "tid=[" << std::this_thread::get_id() << "]";
		break;
	}
	default: break;
	}

	// signals only if it has changed
	publishPortStatusChanged( portState );
}

static std::string canFrameToString(const struct can_frame &f)
{
	std::string result;
	result =  "[id=0x"+CanModuleUtils::toHexString(f.can_id, 3, '0')+" ";
	if (f.can_id & CAN_RTR_FLAG)
		result += "RTR ";
	result+="dlc=" + CanModuleUtils::toString(int(f.can_dlc)) + " data=[";

	for (int i=0; i<f.can_dlc; i++)
		result+= CanModuleUtils::toHexString((unsigned int)f.data[i], 2, '0')+" ";
	result += "]]";

	return result;
}


/**
 *
 * The main control thread function for the CAN update scan manager:
 * a private non-static method, which is called on the object (this)
 * following std::thread C++11 ways.
 *
 * reading from socket, and supervising thread for monitoring the sockets/CAN buses.
 * It takes an object reference (cast) and listens with a
 * select call on that socket/object. The select runs with 1Hz, and if
 * there is nothing to receive it should timeout.
 */
void CSockCanScan::m_CanScanControlThread()
{
	struct can_frame  socketMessage;
	CSockCanScan *p_sockCanScan = this;
	// convenience, not really needed as we call the thread as
	// non-static private member on the object. I like it nevertheless
	// since it explicitly makes a difference between the object (this) and this thread.

	std::string _tid;
	{
		std::stringstream ss;
		ss << std::this_thread::get_id();
		_tid = ss.str();
	}
	MLOGSOCK(TRC, p_sockCanScan) << "created main loop tid= " << _tid;

	p_sockCanScan->m_CanScanThreadRunEnableFlag = true;
	int sock = p_sockCanScan->m_sock;
	{

		MLOGSOCK(INF,p_sockCanScan) << "waiting for first reception on socket " << sock;

		int selectResult = m_selectWrapper();
		if ( selectResult > 0 )
		{
			int numberOfReadBytes = read(m_sock, &socketMessage, sizeof(can_frame));
			MLOGSOCK(INF, this) << "discarding first read on socket " << m_sock
					<< " " << canFrameToString(socketMessage)
					<< "got numberOfReadBytes= " << numberOfReadBytes << " discard them";
		}
	}

	MLOGSOCK(TRC,p_sockCanScan) << "main loop of SockCanScan starting, tid= " << _tid;

	// unblock init of reconnection thread
	MLOGSOCK(TRC,p_sockCanScan) << "init reconnection thread " << p_sockCanScan->getBusName();
	triggerReconnectionThread();

	// initial port state
	fetchAndPublishCanPortState();

	while ( p_sockCanScan->m_CanScanThreadRunEnableFlag ) {

		/**
		 * in socketcan we can just ask the kernel what the CAN port state is without using any hardware.
		 * Since we can do this as often as we want (this loop) we can also hook up a signal to port status changes
		 * For other implementations this has to be taken with care: getting the port status usually means
		 * interrogating the hardware. The hw is used by send/receive already, so a port status can be updated somehow
		 * by looking at the errors when sending and receiving. Additionally, port status can be acquired
		 * directly if neither a send nor a receive have been done since a certain time (timeout 5 seconds?)
		 *
		 * disable for the moment.
		 */
		// fetchAndPublishState();

		int selectResult = m_selectWrapper();

		// the select failed. that is very bad, system problem, but let's continue nevertheless. There is not much we can do
		if ( selectResult < 0 ){
			MLOGSOCK(ERR,p_sockCanScan) << "select() failed: " << CanModuleerrnoToString()
									<< " tid= " << _tid;
			CanModule::ms_sleep( 1000 );

			// report "other error" on port status: this might stem from a kernel/driver problem and not
			// from the hw port itself. But we don't know for sure.
			publishPortStatusChanged( CanModule::CANMODULE_ERROR );
			continue;
		}

		/**
		 * a timeout on the socket, usually. skip the reading.
		 */
		if (selectResult == 0)
		{
			MLOGSOCK(DBG, this) << "listening on " << getBusName() << " socket= " << m_sock << " (got nothing)"<< " tid= " << _tid;

			/**
			 * the reconnect behavior happens in an extra thread so that the sendMessage becomes non blocking. This is
			 * the reception of the message part, and strictly speaking we could do the reconnection due to reception
			 * message timeout also here.
			 *
			 * That is NOT the same as the "select" timeout, of course			 *
			 * For cleanliness, lets use that extra reconnection thread nevertheless here.
			 */
			MLOGSOCK(DBG,p_sockCanScan) << "trigger reconnection thread to check reception timeout " << p_sockCanScan->getBusName();
			triggerReconnectionThread();

			// shall we report that timeout in the port status? yes!
			publishPortStatusChanged( CanModule::CANMODULE_TIMEOUT_OK );
			continue;
		}


		/**
		 * select returned some data, no timeout, read it.
		 */
		int numberOfReadBytes = read(m_sock, &socketMessage, sizeof(can_frame));
		MLOGSOCK(DBG, this) << "read(): " << canFrameToString(socketMessage) << " tid= " << _tid;
		MLOGSOCK(DBG, this) << "got numberOfReadBytes= " << numberOfReadBytes << " tid= " << _tid;;
		if (numberOfReadBytes < 0)
		{
			timeval now = CanModule::convertTimepointToTimeval( std::chrono::high_resolution_clock::now());
			p_sockCanScan->canMessageError(numberOfReadBytes, ("read() failed: " + CanModuleerrnoToString() + " tid= " + _tid).c_str(), now );
			publishPortStatusChanged( CanModule::CANMODULE_WARNING );

			MLOGSOCK(DBG,p_sockCanScan) << "trigger reconnection thread since we only read " << numberOfReadBytes << " <0 bytes from socket " << p_sockCanScan->getBusName();
			triggerReconnectionThread();

			// we should be fine again and continue the thread execution normally
			continue;
		}
		if (numberOfReadBytes <(int) sizeof(struct can_frame)) {
			MLOGSOCK( WRN, this ) << m_channelName << " incomplete frame received, numberOfReadBytes=[" << numberOfReadBytes << "]";

			// we just report the error and continue the thread normally
			// got something, but wrong length and therefore obviously wrong data
			timeval now = CanModule::convertTimepointToTimeval( std::chrono::high_resolution_clock::now());
			p_sockCanScan->canMessageError( numberOfReadBytes, ("read() wrong msg size: " + CanModuleerrnoToString() + " tid= " + _tid).c_str(), now );

			publishPortStatusChanged( CanModule::CANMODULE_WARNING );
			continue;
		}

		/**
		 * message is OK, no timeout in this case
		 */

		// got an error from the socket
		if ( numberOfReadBytes < 0 ) {
			MLOGSOCK(ERR,p_sockCanScan) << "read() error: " << CanModuleerrnoToString()<< " tid= " << _tid;;
			timeval now = CanModule::convertTimepointToTimeval( std::chrono::high_resolution_clock::now());
			p_sockCanScan->canMessageError( numberOfReadBytes, ("read() error: "+CanModuleerrnoToString()).c_str(), now );
			p_sockCanScan->m_canMessageErrorCode = -1;

			publishPortStatusChanged( CanModule::CANMODULE_WARNING );

			/**
			 * if we have an open socket again, and we can read numberOfReadBytes >=0 we
			 * should be fine again and continue the thread execution normally
			 */
			continue;
		}

		// we also detect a CAN error in the can message/frame
		if ( socketMessage.can_id & CAN_ERR_FLAG ) {

			// we retrieve the timestamp and report the error on the signal of this CAN port
			p_sockCanScan->m_canMessageErrorCode = socketMessage.can_id & ~CAN_ERR_FLAG;
			std::string description = CSockCanScan::m_canMessageErrorFrameToString( socketMessage );

			publishPortStatusChanged( CanModule::CANMODULE_WARNING ); // it is just a CAN error flag, should recover

			// lets find out the time by doing an ioctl call
			timeval c_time;
			int ioctlReturn1 = ioctl(sock, SIOCGSTAMP, &c_time);
			if ( ioctlReturn1 ){
				MLOGSOCK(ERR, p_sockCanScan) << "SocketCAN "
						<< p_sockCanScan->getBusName()
						<< " got an error, ioctl timestamp from socket failed as well, setting local time"
						<< " ioctlReturn1 = " << ioctlReturn1;
				c_time = CanModule::convertTimepointToTimeval( std::chrono::high_resolution_clock::now());
			}
			MLOGSOCK(ERR, p_sockCanScan) << "SocketCAN ioctl return: [" << ioctlReturn1
					<< " error frame: [" << description
					<< "], original: [" << canFrameToString(socketMessage) << "]";

			p_sockCanScan->canMessageError( p_sockCanScan->m_canMessageErrorCode, description.c_str(), c_time ); // signal

			continue;
		}

		/**
		 * we have a CAN message, error free, lets digest it
		 */
		fetchAndPublishCanPortState();
		// we should get a decent bus state from socketcan here, no need to bricoler
		// publishPortStatusChanged( CanModule::CanModuleUtils::CANMODULE_OK );

		CanMessage canMessage;
		canMessage.c_rtr = socketMessage.can_id & CAN_RTR_FLAG;


		/**
		 * reformat and buffer the message from the socket.
		 * this actually should exclude more bits
		 */
		canMessage.c_id = socketMessage.can_id & ~CAN_RTR_FLAG;
		int ioctlReturn2 = ioctl(sock,SIOCGSTAMP,&canMessage.c_time);
		if ( ioctlReturn2 ){
			// actually we do not treat that as an error
			MLOGSOCK(TRC, p_sockCanScan) << "SocketCAN "
					<< p_sockCanScan->getBusName()
					<< " ioctl timestamp from socket failed, setting local time"
					<< " ioctlReturn2 = " << ioctlReturn2;
			canMessage.c_time = CanModule::convertTimepointToTimeval( std::chrono::high_resolution_clock::now());
		}

		//MLOGSOCK(TRC, p_sockCanScan) << " SocketCAN ioctl SIOCGSTAMP return: [" << ioctlReturn2 << "]" << " tid= " << _tid;

		// send the CAN message through it's port signal
		canMessage.c_dlc = socketMessage.can_dlc;
		memcpy(&canMessage.c_data[0],&socketMessage.data[0],8);
		//MLOGSOCK(TRC, p_sockCanScan) << " sending message id= " << canMessage.c_id << " through signal with payload";
		p_sockCanScan->canMessageCame( canMessage ); // signal with payload to trigger registered handler
		p_sockCanScan->m_statistics.onReceive( socketMessage.can_dlc );

		// we can reset the reconnectionTimeout here, since we have received a message
		p_sockCanScan->resetTimeoutOnReception();

	} // while ( p_sockCanScan->m_CanScanThreadRunEnableFlag )
	MLOGSOCK(INF,p_sockCanScan) << "main loop of SockCanScan terminated." << " tid= " << _tid;
}


/**
 * Reconnection thread managing the reconnection behavior, per port. The behavior settings can not change during runtime.
 * This thread is initialized after the main thread is up, and then listens on its cond.var as a trigger.
 * Triggers occur in two contexts: sending and receiving problems.
 * If there is a sending problem which lasts for a while (usually) the reconnection thread will be also triggered for each failed sending:
 * the thread will be "hammered" by triggers. ince the reconnection takes some time, many triggers will be lost. That is in fact a desired behavior.
 *
 * The parameters are all atomics for increased thread-safety, even though the documentation about the predicate is unclear on that point. Since
 * atomics just provide a "sequential memory layout" for the variables to prevent race conditions they are good to use for this but the code still has to be threadsafe
 * and reentrant... ;-) Doesn't eat anything anyway on that small scale with scalars only.
 *
 * https://en.cppreference.com/w/cpp/thread/condition_variable/wait
 */
void CSockCanScan::m_CanReconnectionThread()
{
	std::string _tid;
	{
		std::stringstream ss;
		ss << std::this_thread::get_id();
		_tid = ss.str();
	}
	MLOGSOCK(TRC, this ) << "created reconnection thread tid= " << _tid;

	// need some sync to the main thread to be sure it is up and the sock is created: wait first time for init
	waitForReconnectionThreadTrigger();

	/**
	 * lets check the timeoutOnReception reconnect condition. If it is true, all we can do is to
	 * close/open the socket again since the underlying hardware is hidden by socketcan abstraction.
	 * Like his we do not have to pollute the "sendMessage" like for anagate, and that is cleaner.
	 */
	CanModule::ReconnectAutoCondition rcond = getReconnectCondition();
	CanModule::ReconnectAction ract = getReconnectAction();

	MLOGSOCK(TRC, this) << "initialized reconnection thread tid= " << _tid << ", entering loop";
	while ( true ) {

		// wait for sync: need a condition sync to step that thread once: a "trigger".
		MLOGSOCK(TRC, this) << "waiting reconnection thread tid= " << _tid;
		waitForReconnectionThreadTrigger();
		MLOGSOCK(TRC, this)
		<< " reconnection thread tid= " << _tid
		<< " condition "<< reconnectConditionString(rcond)
		<< " action " << reconnectActionString(ract)
		<< " is checked, m_failedSendCountdown= "
		<< m_failedSendCountdown;


		/**
		 * just manage the conditions, and continue/skip if there is nothing to do
		 */
		switch ( rcond ){
		case CanModule::ReconnectAutoCondition::timeoutOnReception: {
			resetSendFailedCountdown();
			if ( !hasTimeoutOnReception() ){
				continue;                  // do nothing
			} else {
				resetTimeoutOnReception(); // do the action
			}
			break;
		}
		case CanModule::ReconnectAutoCondition::sendFail: {
			resetTimeoutOnReception();
			if (m_failedSendCountdown > 0) {
				continue;                   // do nothing
			} else {
				resetSendFailedCountdown(); // do the action
			}
			break;
		}
		// do nothing but keep counter and timeout resetted
		case CanModule::ReconnectAutoCondition::never:
		default:{
			resetSendFailedCountdown();
			resetTimeoutOnReception();
			continue;// do nothing
			break;
		}
		} // switch

		// single bus reset if (send) countdown or the (receive) timeout says so
		switch ( m_reconnectAction ){
		case CanModule::ReconnectAction::singleBus: {

			MLOGSOCK(INF, this) << " reconnect condition " << reconnectConditionString(m_reconnectCondition)
										<< " triggered action " << reconnectActionString(m_reconnectAction);
			close( m_sock );
			int return0 = m_openCanPort();
			MLOGSOCK(TRC, this) << "reconnect openCanPort() ret= " << return0;
			break;
		}

		default: {
			/**
			 * socketcan abstracts away the notion of a "module", and that is the point. Various plugin-orders
			 * should lead to the same device mapping nevertheless. But then we can't
			 * refer to a module and reset all of it's channels easily in linux. Unless we make a big effort and keep
			 * track of which port is on which module, for peak: more udev calls, for systec: we need
			 * to read the module serial number or similar. Maybe there is an elegant way out, but I think
			 * it is not worth it. Use a PDU if you want to reset your systec16. For peak the notion
			 * of "module" is already difficult through socketcan (udev calls needed to identify the modules) and
			 * peak bridges get their power over USB. So in fact testing peak means "rebooting" unless you want
			 * to unplug the USB. Therefore "allBusesOnBridge" as reconnect action is not available for sock.
			 *
			 * CanModule::ReconnectAction::allBusesOnBridge is not implemented for sock
			 */
			MLOGSOCK(WRN, this) << "reconnection action "
					<< (int) m_reconnectAction << reconnectActionString( m_reconnectAction )
					<< " is not available for the socketcan/linux implementation. Check your config & see documentation. No action.";
			break;
		}
		} // switch
	} // while
}

CSockCanScan::~CSockCanScan()
{
	stopBus();
	MLOGSOCK(DBG,this) << __FUNCTION__ <<" closed successfully";
}

int CSockCanScan::m_configureCanBoard(const std::string name,const std::string parameters)
{
	std::string lname = name;

	/**
	 * for PEAK bridges, we have a problem: the local ports are not mapped to the global socketcan
	 * ports in a deterministic way. OPCUA-1735.
	 * So, lets make a udev call here, analyze the result and manipulate the port number so that
	 * it becomes a global port number.
	 *
	 * In order to distinguish between systec (where it works) and peak bridges we use an extended name:
	 * systec: name="sock:can0"
	 * peak: name="sock:can0:device17440"
	 */
	if ( name.find("device") != std::string::npos ) {
		LOG(Log::INF, m_logItHandleSock) << "found extended port identifier for PEAK " << name;
		// get a mapping: do all the udev calls in the constructor of the singleton
		std::string sockPort = udevanalyserforpeak_ns::UdevAnalyserForPeak::peakExtendedIdentifierToSocketCanDevice( name );
		LOG(Log::INF, m_logItHandleSock) << "portIdentifierToSocketCanDevice: name= " << name << " sockPort= " << sockPort;

		// show the whole map
		udevanalyserforpeak_ns::UdevAnalyserForPeak::showMap();

		/**
		 * now, we manipulate the lname to reflect the absolute port which we found out. We do the rewiring at initialisation, like this we don't
		 * care anymore during runtime.
		 */
		lname = std::string("sock:") + sockPort;
		LOG(Log::INF, m_logItHandleSock) << "peak *** remapping extended port ID= " << name << " to global socketcan portID= " << lname << " *** ";
	}
	std::vector<std::string> parset;
	parset = parseNameAndParameters( lname, parameters );
	m_channelName = parset[1];
	MLOGSOCK(TRC, this) << "m_channelName= " << m_channelName ;
	return m_openCanPort();
}

/**
 * Obtains a SocketCAN socket and opens it.
 *  The name of the port and parameters should have been specified by preceding call to configureCanboard()
 *
 *  @returns less than zero in case of error, otherwise success

 * stop, set bitrate, start a CAN port, open a socket for it, set the socket to CAN,
 * bind it and check any errors
 */
int CSockCanScan::m_openCanPort()
{
	/* Determine if it was requested through params to configure the port (baudrate given) or just to open it ("Unspecified")*/

	if (!m_CanParameters.m_dontReconfigure)
	{
		int returnCode = can_do_stop(m_channelName.c_str());
		if (returnCode < 0)
		{
			MLOGSOCK(ERR,this) << "Error while can_do_stop(): " << CanModuleerrnoToString();
			return -1;
		}
		returnCode = can_set_bitrate(m_channelName.c_str(),m_CanParameters.m_lBaudRate);
		if (returnCode < 0)
		{
			MLOGSOCK(ERR,this) << "Error while can_set_bitrate(): " << CanModuleerrnoToString();
			return -1;
		}
		returnCode = can_do_start(m_channelName.c_str());
		if (returnCode < 0)
		{
			MLOGSOCK(ERR,this) << "Error while can_do_start(): " << CanModuleerrnoToString();
			return -1;
		}
	} else {
		MLOGSOCK(INF,this) << "did NOT reconfigure (stop, set bitrate, start) the CAN port";
		struct can_bittiming bt;
		int ret = can_get_bittiming (m_channelName.c_str(), &bt);
		if ( ret == 0 && bt.bitrate > 0 ){
			MLOGSOCK(INF,this) << "successfully read bitrate from socketcan, statistics will work, bitrate= " << bt.bitrate;
			m_CanParameters.m_lBaudRate = bt.bitrate;
		} else {
			MLOGSOCK(WRN,this) << "could not read bitrate from socketcan, statistics will not fully work, bitrate= " << bt.bitrate;
		}
	}

	m_sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (m_sock < 0)
	{
		MLOG(ERR,this) << "Error while opening socket: " << CanModuleerrnoToString();
		return -1;
	}
	struct ifreq ifr;
	memset( &ifr.ifr_name, 0, sizeof(ifr.ifr_name ));
	if ( m_channelName.length() > sizeof ifr.ifr_name-1)
	{
		MLOGSOCK(ERR,this) << "Given name of the port exceeds operating-system imposed limit";
		return -1;
	}
	MLOGSOCK(INF,this) << " ioctl setting channel name to " << m_channelName;
	strncpy(ifr.ifr_name, m_channelName.c_str(), sizeof(ifr.ifr_name)-1);

	/** if this fails, crate and configure your socketcan devices properly using
	 *ip link set can0 type can bitrate 250000
	 * ifconfig can0 up
	 *
	 * Retrieve the interface index of the interface into ifr_ifindex.
	 *  https://www.man7.org/linux/man-pages/man7/netdevice.7.html
	 */
	if (ioctl(m_sock, SIOCGIFINDEX, &ifr) < 0)
	{
		MLOGSOCK(ERR,this) << "ioctl SIOCGIFINDEX failed. " << CanModuleerrnoToString();
		return -1;
	}


	can_err_mask_t err_mask = 0x1ff;
	if( setsockopt(m_sock, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof err_mask) < 0 )
	{
		MLOGSOCK(ERR,this) << "setsockopt() failed: " << CanModuleerrnoToString();
		return -1;
	}

	struct sockaddr_can socketAdress;
	socketAdress.can_family = AF_CAN;
	socketAdress.can_ifindex = ifr.ifr_ifindex;

	// server listening on socket
	if (::bind(m_sock, (struct sockaddr *)&socketAdress, sizeof(socketAdress)) < 0)
	{
		MLOGSOCK(ERR,this) << "While bind(): " << CanModuleerrnoToString();
		return -1;
	}

	// initial state of the port
	fetchAndPublishCanPortState ();
	updateInitialError();
	m_statistics.setTimeSinceOpened();
	return m_sock;
}



/**
 * Method that sends a message trough the can bus channel. If the method createBUS was not
 * called before this, sendMessage will fail, as there is no
 * can bus channel to send a message through.
 *
 * @param cobID Identifier that will be used for the message.
 * @param len Length of the message. If the message is bigger than 8 characters, it will be split into separate 8 characters messages.
 * @param message Message to be sent trough the can bus.
 * @param rtr is the message a remote transmission request?
 * @return Was the initialisation process successful?
 *
 * OPCUA-2604: sendMessage must be non blocking. The reconnection behavior therefore must be managed in a separate thread.
 *
 * returns: true for success, otherwise false
 */
/* virtual */ bool CSockCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	int messageLengthToBeProcessed;

	// we should skip this if the port is "closed" already
	if ( !m_CanScanThreadRunEnableFlag ){
		MLOGSOCK(TRC,this) << __FUNCTION__ << " bus is already closed, sending refused";
		return false;
	}

	struct can_frame canFrame = CSockCanScan::emptyCanFrame();
	if (len > 8) {
		messageLengthToBeProcessed = 8;
		MLOGSOCK(DBG,this) << "The Length is more then 8 bytes: " << std::dec << len;
		return false;
	} else {
		messageLengthToBeProcessed = len;
	}

	canFrame.can_dlc = messageLengthToBeProcessed;
	memcpy(canFrame.data, message, len);
	canFrame.can_id = cobID;
	if (rtr) {
		canFrame.can_id |= CAN_RTR_FLAG;
	}

	// throttle the speed to avoid frame losses. we just wait the minimum time needed
	if ( m_sendThrottleDelay > 0 ) {
		m_now = boost::posix_time::microsec_clock::local_time();
		int remaining_sleep_us = m_sendThrottleDelay - (m_now - m_previous).total_microseconds();
		if ( remaining_sleep_us > m_sendThrottleDelay ){
			remaining_sleep_us = m_sendThrottleDelay;
		}
		if ( remaining_sleep_us > 0 ){
			us_sleep( remaining_sleep_us );
		}
		m_previous = boost::posix_time::microsec_clock::local_time();
	}

	// we can send now
	bool success = m_writeWrapper(&canFrame);
	if ( success ) {
		// m_statistics.onTransmit( canFrame.can_dlc );
		m_statistics.onTransmit( canFrame.can_dlc );
		m_statistics.setTimeSinceTransmitted();
		resetSendFailedCountdown();
	} else {
		// trigger the reconnection thread once since there was a send fail. If this repeats N times successively, the
		// m_triggerCounter == 0 and the reconnection thread will do sth.
		MLOGSOCK(WRN,this) << "sendMessage fail detected. Trigger reconnection thread and return (no block).";

		decreaseSendFailedCountdown();
		triggerReconnectionThread();
	}
	return success;
}


int CSockCanScan::m_selectWrapper ()
{
	fd_set readSet, exceptSet;
	FD_ZERO( &readSet );
	FD_SET( m_sock, &readSet );

	FD_ZERO( &exceptSet );
	FD_SET( m_sock, &exceptSet );

	timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	return select( m_sock+1, &readSet, nullptr, &exceptSet, &timeout ); // TODO: we should have two FDSETs for this select call
}

/**
 * write can frame to socket
 * returns:
 * true=OK
 * false= error, either buffer fault or less bytes than expected or other problem
 */
bool CSockCanScan::m_writeWrapper (const can_frame* frame)
{
	ssize_t numberOfWrittenBytes;
	numberOfWrittenBytes = write(m_sock, frame, sizeof (struct can_frame));
	if ( numberOfWrittenBytes < 0 ) {
		if ( errno == ENOBUFS ) {
			MLOGSOCK(ERR, this) << "writeWrapper: ENOBUFS; waiting a jiffy [100ms]...";
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			return false;
		} else {
			MLOGSOCK(ERR, this) << "writeWrapper write: " << CanModuleerrnoToString();
			return false;
		}
	} else if ((size_t)numberOfWrittenBytes < sizeof (struct can_frame)) {
		// numberOfWrittenBytes is for sure positive
		MLOGSOCK(ERR,this) << "writeWrapper: sent less bytes than expected";
		return false;
	} else {
		return true;
	}
	MLOGSOCK(ERR,this) << "writeWrapper: something went seriously wrong";
	return false; // normally never reached
}

/**
 * Method that sends a remote request trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
 * we treat the remote request like all other send messages, including stats, error signals and reconnection on the bus.
 * @param cobID: Identifier that will be used for the request.
 * @return: Was the initialisation process successful?
 *
 * Used in the CANopen NG server and other applications.
 */
/* virtual */ bool CSockCanScan::sendRemoteRequest(short cobID)
{
	struct can_frame canFrame (emptyCanFrame());
	canFrame.can_id = cobID + CAN_RTR_FLAG	;

	bool success = m_writeWrapper(&canFrame);
	if ( success ) {
		//m_statistics.onTransmit( canFrame.can_dlc );
		m_statistics.onTransmit( canFrame.can_dlc );
		m_statistics.setTimeSinceTransmitted();
		resetSendFailedCountdown();
	} else {
		// trigger the reconnection thread once since there was a send fail. If this repeats N times successively, the
		// m_triggerCounter == 0 and the reconnection thread will do sth.
		MLOGSOCK(WRN,this) << "sendMessage fail detected. Trigger reconnection thread and return (no block).";

		decreaseSendFailedCountdown();
		triggerReconnectionThread();
	}
	return ( success );
}


/**
 * Method that initializes a can bus channel. The following methods called upon the same
 * object will be using this initialized channel.
 *
 * @param name = 2 parameters separated by ":", like "n0:n1"
 * 		* n0 = "sock" for sockets linux, used by systec and peak
 * 		* n1 = CAN port number on the module, can be prefixed with "can"
 * 		* ex.: "sock:can1" speaks to port 1 on systec or peak module
 * 		* ex.: "sock:1" works as well
 *
 * @param parameters one parameter: "p0", positive integer
 * 				* "Unspecified" (or empty): using defaults = "125000" // params missing
 * 				* p0: bitrate: 50000, 100000, 125000, 250000, 500000, 1000000 bit/s, other values might be allowed by the module
 *				*  i.e. "250000"
 *
 * @return Was the initialization process successful?
 *
 * dont create a main thread for the same bus twice: if it exists already, just configure the board again
 * this protects agains having multiple threads if a given port ports is opened several times: protects
 * against erro/unusual runtime. If the connection is closed, it's main thread is stopped and joined, and
 * the port is erased from the connection map. when the same port is opened again later on, a (new)
 * main thread is created, and the connection is again added to the map.
 */
int CSockCanScan::createBus(const std::string name, const std::string parameters, float factor )
{
	m_sendThrottleDelay = (int) factor;
	MLOGSOCK(TRC, this) << "the frame sending delay is " << m_sendThrottleDelay << " us";
	return( createBus( name, parameters) );
}
/* virtual */ int CSockCanScan::createBus(const std::string name, const std::string parameters)
{
	m_gsig = GlobalErrorSignaler::getInstance();

	/** LogIt: initialize shared lib. The logging levels for the component logging is kept, we are talking still to
	 * the same master object "from the exe". We get the logIt ptr
	 * acquired down from the superclass, which keeps it as a static, and being itself a shared lib. we are inside
	 * another shared lib - 2nd stage - so we need to Dll initialize as well. Since we have many CAN ports we just acquire
	 * the handler to go with the logIt object and keep that as a static. we do not do per-port component logging.
	 * we do, however, stamp the logging messages specifically for each vendor using the macro.
	 */
	LogItInstance *logIt = CCanAccess::getLogItInstance();
	if ( logIt != NULL ){
		if (!Log::initializeDllLogging( logIt )){
			std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
					<< " could not DLL init remote LogIt instance " << std::endl;
		}
		logIt->getComponentHandle( CanModule::LogItComponentName, m_logItHandleSock );
		LOG(Log::INF, m_logItHandleSock ) << CanModule::LogItComponentName << " Dll logging initialized OK";
	} else {
		std::stringstream msg;
		msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " LogIt instance is NULL";
		std::cout << msg.str() << std::endl;
		m_gsig->fireSignal( 002, msg.str().c_str() );
	}

	/**
	 * lets get clear about the Logit components and their levels at this point
	 */
	std::map<Log::LogComponentHandle, std::string> log_comp_map = Log::getComponentLogsList();
	std::map<Log::LogComponentHandle, std::string>::iterator it;
	MLOGSOCK(TRC,this)<< " *** Lnb of LogIt components= " << log_comp_map.size() << std::endl;
	for ( it = log_comp_map.begin(); it != log_comp_map.end(); it++ )
	{
		Log::LOG_LEVEL level;
		Log::getComponentLogLevel( it->first, level);
		MLOGSOCK(TRC,this) << " *** " << " LogIt component " << it->second << " level= " << level;
	}

	// protect against creating the same bus twice
	bool skip = false;
	{

		std::lock_guard<std::mutex> busMapScopedLock( sockReconnectMutex );

		std::map<std::string, std::string>::iterator it = CSockCanScan::m_busMap.find( name );
		if (it == CSockCanScan::m_busMap.end()) {
			CSockCanScan::m_busMap.insert ( std::pair<std::string, std::string>(name, parameters) );
			m_busName = name;
			MLOGSOCK(TRC,this) << "added to busMap: [" << name << "] with parameters [" << parameters << "]";
		} else {
			skip = true;
		}
	}
	if ( skip ){
		LOG(Log::WRN) << __FUNCTION__ << " bus exists already [" << name << ", " << parameters << "], skipping board config";
		return( 1 );
	}

	m_CanScanThreadRunEnableFlag = true;
	m_sock = m_configureCanBoard(name,parameters);
	if (m_sock < 0) {
		MLOGSOCK(ERR,this) << "Could not create bus [" << name << "] with parameters [" << parameters << "]";
		// take it out from the map again
		std::map<std::string, std::string>::iterator it = CSockCanScan::m_busMap.find( name );
		if (it != CSockCanScan::m_busMap.end()) {
			CSockCanScan::m_busMap.erase ( it );
			m_busName = "";
			MLOGSOCK(TRC,this) << "removed from busMap: [" << name << "] with parameters [" << parameters << "]";
		}

		return -1;
	}
	MLOGSOCK(TRC,this) << "Created bus with parameters [" << parameters << "], starting main loop";

	// start a thread on a private non-static member function. We don't need any arguments since
	// the thread is run on the object anyway.
	// m_hCanScanThread = new std::thread( &CSockCanScan::CanScanControlThread, this, "test" );
	m_hCanScanThread = new std::thread( &CSockCanScan::m_CanScanControlThread, this);
	MLOGSOCK(TRC,this) << "created main thread m_idCanScanThread";

	/**
	 * start a reconnection thread
	 */
	m_hCanReconnectionThread = new std::thread( &CSockCanScan::m_CanReconnectionThread, this);
	return( 0 );
}

/**
 * Transforms an error frame into an error message (string format):
 * Provides textual representation of SocketCAN error.
 */
std::string CSockCanScan::m_canMessageErrorFrameToString(const struct can_frame &canFrame)
{
	std::string result;
	// Code below is taken from SysTec documentation
	if (canFrame.can_id & CAN_ERR_FLAG)
	{
		if (canFrame.can_id & CAN_ERR_TX_TIMEOUT)
		{
			result += "TX_timeout ";
		}
		if (canFrame.can_id & CAN_ERR_LOSTARB)
		{
			result += "Arbitration_lost ";
		}
		if (canFrame.can_id & CAN_ERR_CRTL)
		{
			result += "Controller_state=( ";
			if (canFrame.data[1] & CAN_ERR_CRTL_RX_WARNING)
			{
				result += "RX_warning ";
			}
			if (canFrame.data[1] & CAN_ERR_CRTL_TX_WARNING)
			{
				result += "TX_warning ";
			}
			if (canFrame.data[1] & CAN_ERR_CRTL_RX_PASSIVE)
			{
				result += "RX_passive ";
			}
			if (canFrame.data[1] & CAN_ERR_CRTL_TX_PASSIVE)
			{
				result += "TX_passive ";
			}
			result += ") ";
		}
		if (canFrame.can_id & CAN_ERR_PROT)
		{
			result += "Protocol_violation=( type=" + CanModuleUtils::toHexString(canFrame.data[2]) ;
			if (canFrame.data[2] & CAN_ERR_PROT_ACTIVE )
			{
				result += " Error_active(recovered) ";
			}
			result += ") ";
			// TODO we could print in more details what sort of violation is that
		}
		if (canFrame.can_id & CAN_ERR_TRX)
		{
			result += "Transceiver status=( " + CanModuleUtils::toHexString(canFrame.data[4]) + ") ";
			// TODO we could print in more details what sort of TRX status that is
		}
		if (canFrame.can_id & CAN_ERR_ACK)
		{
			result += "No_ACK ";
		}
		if (canFrame.can_id & CAN_ERR_BUSOFF)
		{
			result += "Bus_off ";
		}
		if (canFrame.can_id & CAN_ERR_BUSERROR)
		{
			result += "Bus_error ";
		}
		if (canFrame.can_id & CAN_ERR_RESTARTED)
		{
			result += "Controller_restarted ";
		}
	}
	else
	{
		result = "No error";
	}
	return result;
}

void CSockCanScan::m_clearErrorMessage()
{
	std::string errorMessage = "";
	timeval c_time;
	int ioctlReturn = ioctl(m_sock, SIOCGSTAMP, &c_time);//TODO: Return code is not checked
	if ( ioctlReturn ){
		MLOGSOCK(ERR, this) << "SocketCAN "
				<< getBusName()
				<< " ioctl timestamp from socket failed, setting local time"
				<< " ioctlReturn = " << ioctlReturn;
		c_time = CanModule::convertTimepointToTimeval( std::chrono::high_resolution_clock::now());
	}
	MLOGSOCK(TRC,this) << "ioctlReturn= " << ioctlReturn;
	canMessageError(0, errorMessage.c_str(), c_time);
}

/**
 * notify the main thread to finish and delete the bus from the map of connections
 */
/* virtual */ void CSockCanScan::stopBus ()
{
	if ( m_CanScanThreadRunEnableFlag == false ){
		MLOGSOCK(DBG,this) << "bus is already closed & thread finished, skipping";
		return;
	}
	MLOGSOCK(DBG,this) << __FUNCTION__ << " m_busName= " <<  m_busName << " try joining thread...";
	// notify the thread that it should finish.
	m_CanScanThreadRunEnableFlag = false;
	{
		std::lock_guard<std::mutex> busMapScopedLock( sockReconnectMutex );

		std::map<std::string, std::string>::iterator it = CSockCanScan::m_busMap.find( m_busName );
		if (it != CSockCanScan::m_busMap.end()) {
			m_hCanScanThread->join();
			CSockCanScan::m_busMap.erase ( it );
			m_busName = "nobus";
		} else {
			MLOGSOCK(DBG,this) << "bus not found in map, is already closed & thread finished, skipping";
		}
	}
}

/* virtual */ void CSockCanScan::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived ( m_CanParameters.m_lBaudRate );
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}

/**
 * Report an error when opening a can port
 */
void CSockCanScan::updateInitialError ()
{
	if ( m_canMessageErrorCode == 0 ) {
		m_clearErrorMessage();
	} else {
		timeval now = CanModule::convertTimepointToTimeval( std::chrono::high_resolution_clock::now());
		canMessageError( m_canMessageErrorCode, "Initial port state: error", now );
	}
}


/* static */ can_frame CSockCanScan::emptyCanFrame( void ){
	can_frame f;
	f.can_dlc = 0;
	f.can_id = 0;
	for ( int i = 0; i < 8; i++){
		f.data[ i ] = 0;
	}
	return(f);
}


/**
 * return socketcan port status in vendor unified format (with implementation nibble set), from can_netlink.h
 * do not start a new statistics run for that.
 * enum can_state {
 * CAN_STATE_ERROR_ACTIVE = 0,	 RX/TX error count < 96
 * CAN_STATE_ERROR_WARNING,	 RX/TX error count < 128
 * CAN_STATE_ERROR_PASSIVE,	 RX/TX error count < 256
 * CAN_STATE_BUS_OFF,		 RX/TX error count >= 256
 * CAN_STATE_STOPPED,		 Device is stopped
 * CAN_STATE_SLEEPING,		 Device is sleeping
 * CAN_STATE_MAX
 * };
 *
 * update the CAN bus status IFLA from socketcan and make up a CanModule
 * status bitpattern out from this.
 *
 * int can_get_restart_ms(const char *name, __u32 *restart_ms);
 * int can_get_bittiming(const char *name, struct can_bittiming *bt);
 * int can_get_ctrlmode(const char *name, struct can_ctrlmode *cm);
 * int can_get_state(const char *name, int *state);
 * int can_get_clock(const char *name, struct can_clock *clock);
 * int can_get_bittiming_const(const char *name, struct can_bittiming_const *btc);
 * int can_get_berr_counter(const char *name, struct can_berr_counter *bc);
 * int can_get_device_stats(const char *name, struct can_device_stats *cds);
 *
 */
/* virtual */ uint32_t CSockCanScan::getPortStatus(){
	int _state;
	can_get_state( m_channelName.c_str(), &_state );
	return( _state | CANMODULE_STATUS_BP_SOCK );
};

