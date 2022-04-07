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

/* static */ std::map<string, string> CSockCanScan::m_busMap;
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
							m_errorCode(-1),
							m_hCanScanThread( NULL ),
							m_busName("nobus"),
							m_logItHandleSock(0)
{
	m_statistics.setTimeSinceOpened();
	m_statistics.beginNewRun();
	m_failedSendCountdown = m_maxFailedSendCount;
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
void CSockCanScan::CanScanControlThread()
{
	struct can_frame  socketMessage;
	CSockCanScan *p_sockCanScan = this;
	// convenience, not really needed as we call the thread as
	// non-static private member on the object

	std::string _tid;
	{
		std::stringstream ss;
		ss << this_thread::get_id();
		_tid = ss.str();
	}
	MLOGSOCK(TRC, p_sockCanScan) << "created main loop tid= " << _tid;

	p_sockCanScan->m_CanScanThreadRunEnableFlag = true;
	int sock = p_sockCanScan->m_sock;
	{
		// discard first read
		fd_set set;
		FD_ZERO( &set );
		FD_SET( sock, &set );

		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		MLOGSOCK(INF,p_sockCanScan) << "waiting for first reception on socket " << sock;

		int selectResult = select( sock+1, &set, 0, &set, &timeout );
		if ( selectResult > 0 ) {
			int numberOfReadBytes = read(sock, &socketMessage, sizeof(can_frame));
			MLOGSOCK(INF,p_sockCanScan) << "discarding first read on socket " << sock
					<< " " << canFrameToString(socketMessage)
					<< "got numberOfReadBytes= " << numberOfReadBytes << " discard them";
		}
	}

	MLOGSOCK(TRC,p_sockCanScan) << "main loop of SockCanScan starting, tid= " << _tid;

	// unblock init of reconnection thread
	MLOGSOCK(TRC,p_sockCanScan) << "init reconnection thread " << p_sockCanScan->getBusName();
	triggerReconnectionThread();

	while ( p_sockCanScan->m_CanScanThreadRunEnableFlag ) {
		fd_set set;
		FD_ZERO( &set );
		FD_SET( sock, &set );

		// p_sockCanScan->updateBusStatus();

		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int selectResult = select( sock+1, &set, 0, &set, &timeout );

		/**
		 * the select failed. that is very bad, system problem, but let's continue nevertheless.
		 * There is not much we can do
		 */
		if ( selectResult < 0 ){
			MLOGSOCK(ERR,p_sockCanScan) << "select() failed: " << CanModuleerrnoToString()
									<< " tid= " << _tid;
			{
				struct timespec tim, tim2;
				tim.tv_sec = 1;
				tim.tv_nsec = 0;
				if(nanosleep(&tim , &tim2) < 0 ) {
					MLOGSOCK(ERR,p_sockCanScan) << "Waiting 1s failed (nanosleep) tid= " << _tid;
				}
			}
			continue;
		}


		/**
		 * select returned, either on the timeout or with something on that socket/object.
		 * Assumption: at this moment sock holds meaningful value.
		 * Now select result >=0 so it was either nothing received (timeout) or something received
		 */
		if ( p_sockCanScan->m_errorCode ) {
			/** The preceding call took either 'timeout' time, or there is frame received --
			 * perfect time to attempt to clean error frame.
			 */
			int status_socketcan = -1;
			int ret = can_get_state( p_sockCanScan->m_channelName.c_str(), &status_socketcan);
			if (ret != 0) {
				MLOGSOCK(ERR,p_sockCanScan) << "can_get_state failed, " << " tid= " << _tid;
			} else {
				if (status_socketcan == 0) {
					p_sockCanScan->m_errorCode = 0;
					timeval t = convertTimepointToTimeval( std::chrono::system_clock::now());
					p_sockCanScan->canMessageError( p_sockCanScan->m_errorCode, "CAN port is recovered", t );
				}
			}
		}

		/**
		 * select reports that it has got something, so no timeout in this case
		 */
		if ( selectResult > 0 ) {
			int numberOfReadBytes = read(sock, &socketMessage, sizeof(can_frame));
			MLOGSOCK(DBG,p_sockCanScan) << "read(): " << canFrameToString(socketMessage) << " tid= " << _tid;
			MLOGSOCK(DBG,p_sockCanScan) << "got numberOfReadBytes= " << numberOfReadBytes << " tid= " << _tid;;

			// got an error from the socket
			if (numberOfReadBytes < 0) {
				MLOGSOCK(ERR,p_sockCanScan) << "read() error: " << CanModuleerrnoToString()<< " tid= " << _tid;;
				timeval now = convertTimepointToTimeval( std::chrono::system_clock::now());
				p_sockCanScan->canMessageError( numberOfReadBytes, ("read() error: "+CanModuleerrnoToString()).c_str(), now );
				p_sockCanScan->m_errorCode = -1;


				// try close/opening on faults while port is active. This was a system error
				do {
					MLOGSOCK(INF,p_sockCanScan) << "Waiting 10000ms."<< " tid= " << _tid;
					{
						struct timespec tim, tim2;
						tim.tv_sec = 10;
						tim.tv_nsec = 0;
						if(nanosleep(&tim , &tim2) < 0 ) {
							MLOG(ERR,p_sockCanScan) << "Waiting 10000ms failed (nanosleep)"<< " tid= " << _tid;
						}
					}
					if ( sock > 0 )	{
						// try closing the socket
						MLOGSOCK(INF,p_sockCanScan) << "Closing socket."<< " tid= " << _tid;
						if (close(sock) < 0) {
							MLOGSOCK(ERR,p_sockCanScan) << "Socket close error!"<< " tid= " << _tid;
						} else {
							sock = -1;
						}
					}

					/**
					 * try to re-open the socket.
					 * only reopen the socket if we still need it.
					 * If we need to shutdown the thread, leave it closed and
					 * terminate main loop.
					 */
					if ( p_sockCanScan->m_CanScanThreadRunEnableFlag ) {
						MLOGSOCK(INF,p_sockCanScan) << " tid= " << _tid << " Now port will be reopened.";
						if ((sock = p_sockCanScan->openCanPort()) < 0) {
							MLOGSOCK(ERR,p_sockCanScan) << " tid= " << _tid << "openCanPort() failed.";
						} else {
						 MLOGSOCK(INF,p_sockCanScan) << " tid= " << _tid << "Port reopened.";
						}

						// the reception handler is still connected to it's signal, by the calling task

					} else {
					 MLOGSOCK(INF,p_sockCanScan) << " tid= " << _tid << "Leaving Port closed, thread and port needed any more.";
					}
				} // do...while ... we still have an error
				while ( p_sockCanScan->m_CanScanThreadRunEnableFlag && sock < 0 );


				/**
				 * if we have an open socket again, and we can read numberOfReadBytes >=0 we
				 * should be fine again and continue the thread execution normally
				 */
				continue;
			}

			// got something, but wrong length and therefore obviously wrong data
			if (numberOfReadBytes <(int) sizeof(struct can_frame)) {
				MLOGSOCK( WRN, p_sockCanScan ) << p_sockCanScan->m_channelName.c_str() << " incomplete frame received, numberOfReadBytes=[" << numberOfReadBytes << "]";

				// we just report the error and continue the thread normally
				continue;
			}

			// detected a CAN error
			if (socketMessage.can_id & CAN_ERR_FLAG) {

				// we retrieve the timestamp and report the CAN port
				p_sockCanScan->m_errorCode = socketMessage.can_id & ~CAN_ERR_FLAG;
				std::string description = CSockCanScan::errorFrameToString( socketMessage );
				timeval c_time;
				int ioctlReturn1 = ioctl(sock, SIOCGSTAMP, &c_time);
				if ( ioctlReturn1 ){
					MLOGSOCK(ERR, p_sockCanScan) << "SocketCAN "
							<< p_sockCanScan->getBusName()
							<< " got an error, ioctl timestamp from socket failed as well, setting local time"
							<< " ioctlReturn1 = " << ioctlReturn1;
					c_time = convertTimepointToTimeval( std::chrono::system_clock::now());
				}
				MLOGSOCK(ERR, p_sockCanScan) << "SocketCAN ioctl return: [" << ioctlReturn1
						<< " error frame: [" << description
						<< "], original: [" << canFrameToString(socketMessage) << "]";
				p_sockCanScan->canMessageError( p_sockCanScan->m_errorCode, description.c_str(), c_time );

				// we have tried to get the error, and we continue thread normally
				continue;
			}

			/**
			 * we have a CAN message, error free, lets digest it
			 */
			CanMessage canMessage;
			canMessage.c_rtr = socketMessage.can_id & CAN_RTR_FLAG;

			/**
			 * OPCUA-2607. Lets just stop filtering RTR out. This is only done here
			 * anyway, all other vendor implementations don't care.
			 */
#if 0
			// remote flag: ignore frame
			if (canMessage.c_rtr) {
				MLOGSOCK(TRC, p_sockCanScan) << " Got a remote CAN message, skipping"<< " tid= " << _tid;
				continue;
			}
#else
			// MLOGSOCK(TRC, p_sockCanScan) << " Got a remote CAN message "<< " tid= " << _tid;
#endif

			/**
			 * reformat and buffer the message from the socket.
			 * this actually should exclude more bits
			 */
			canMessage.c_id = socketMessage.can_id & ~CAN_RTR_FLAG;
			int ioctlReturn2 = ioctl(sock,SIOCGSTAMP,&canMessage.c_time);
			if ( ioctlReturn2 ){
				MLOGSOCK(ERR, p_sockCanScan) << "SocketCAN "
						<< p_sockCanScan->getBusName()
						<< " ioctl timestamp from socket failed, setting local time"
						<< " ioctlReturn2 = " << ioctlReturn2;
				canMessage.c_time = convertTimepointToTimeval( std::chrono::system_clock::now());
			}

			MLOGSOCK(TRC, p_sockCanScan) << " SocketCAN ioctl SIOCGSTAMP return: [" << ioctlReturn2 << "]" << " tid= " << _tid;
			canMessage.c_dlc = socketMessage.can_dlc;
			memcpy(&canMessage.c_data[0],&socketMessage.data[0],8);
			MLOGSOCK(TRC, p_sockCanScan) << " sending message id= " << canMessage.c_id << " through signal with payload";
			p_sockCanScan->canMessageCame( canMessage ); // signal with payload to trigger registered handler
			p_sockCanScan->m_statistics.onReceive( socketMessage.can_dlc );

			// we can reset the reconnectionTimeout here, since we have received a message
			p_sockCanScan->resetTimeoutOnReception();

		} else {
			/**
			 * the select got nothing to read, this was just a timeout, all is fine. Unless the reconnect timeout has something to do.
			 */
			MLOGSOCK(DBG,p_sockCanScan) << "listening on " << p_sockCanScan->getBusName()
									<< " socket= " << p_sockCanScan->m_sock << " (got nothing)"<< " tid= " << _tid;


			/**
			 * the reconnect behavior happens in an extra thread so that the sendMessage becomes non blocking. This is
			 * the reception of the message part, and strictly speaking we could do the reconnection due to reception
			 * message timeout also here.
			 *
			 * That is NOT the same as the "select" timeout, of course.
			 *
			 * For cleanliness, lets use that extra reconnection thread nevertheless here.
			 */
			MLOGSOCK(DBG,p_sockCanScan) << "trigger reconnection thread to check reception timeout " << p_sockCanScan->getBusName();
			triggerReconnectionThread();
		} // else..select showed timeout
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
void CSockCanScan::CanReconnectionThread()
{
	std::string _tid;
	{
		std::stringstream ss;
		ss << this_thread::get_id();
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

			MLOGSOCK(INF, this) << " reconnect condition " << CCanAccess::reconnectConditionString(m_reconnectCondition)
								<< " triggered action " << CCanAccess::reconnectActionString(m_reconnectAction);
			close( m_sock );
			int return0 = openCanPort();
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

int CSockCanScan::configureCanBoard(const string name,const string parameters)
{
	string lname = name;

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
	if ( name.find("device") != string::npos ) {
		LOG(Log::INF, m_logItHandleSock) << "found extended port identifier for PEAK " << name;
		// get a mapping: do all the udev calls in the constructor of the singleton
		string sockPort = udevanalyserforpeak_ns::UdevAnalyserForPeak::peakExtendedIdentifierToSocketCanDevice( name );
		LOG(Log::INF, m_logItHandleSock) << "portIdentifierToSocketCanDevice: name= " << name << " sockPort= " << sockPort;

		// show the whole map
		udevanalyserforpeak_ns::UdevAnalyserForPeak::showMap();

		/**
		 * now, we manipulate the lname to reflect the absolute port which we found out. We do the rewiring at initialisation, like this we don't
		 * care anymore during runtime.
		 */
		lname = string("sock:") + sockPort;
		LOG(Log::INF, m_logItHandleSock) << "peak *** remapping extended port ID= " << name << " to global socketcan portID= " << lname << " *** ";
	}
	vector<string> parset;
	parset = parseNameAndParameters( lname, parameters );
	m_channelName = parset[1];
	MLOGSOCK(TRC, this) << "m_channelName= " << m_channelName ;
	return openCanPort();
}

/**
 * Obtains a SocketCAN socket and opens it.
 *  The name of the port and parameters should have been specified by preceding call to configureCanboard()
 *
 *  @returns less than zero in case of error, otherwise success

 * stop, set bitrate, start a CAN port, open a socket for it, set the socket to CAN,
 * bind it and check any errors
 */
int CSockCanScan::openCanPort()
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
	}

	m_sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (m_sock < 0)
	{
		MLOG(ERR,this) << "Error while opening socket: " << CanModuleerrnoToString();
		return -1;
	}
	struct ifreq ifr;
	memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
	if (m_channelName.length() > sizeof ifr.ifr_name-1)
	{
		MLOGSOCK(ERR,this) << "Given name of the port exceeds operating-system imposed limit";
		return -1;
	}
	strncpy(ifr.ifr_name, m_channelName.c_str(), sizeof(ifr.ifr_name)-1);

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

	// Fetch initial state of the port
	int ret=can_get_state( m_channelName.c_str(), &m_errorCode );
	if ( ret ) {
		m_errorCode = ret;
		MLOGSOCK(ERR,this) << "can_get_state() failed with error code " << ret << ", it was not possible to obtain initial port state";
	}
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
bool CSockCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{

	// we should skip this if the port is "closed" already
	if ( !m_CanScanThreadRunEnableFlag ){
		MLOGSOCK(TRC,this) << __FUNCTION__ << " bus is already closed, sending refused";
		return false;
	}

	bool ret = true;
	int messageLengthToBeProcessed;
	struct can_frame canFrame = CSockCanScan::emptyCanFrame();
	if (len > 8) {
		messageLengthToBeProcessed = 8;
		MLOGSOCK(DBG,this) << "The Length is more then 8 bytes: " << std::dec << len;
	} else {
		messageLengthToBeProcessed = len;
	}
	canFrame.can_dlc = messageLengthToBeProcessed;
	memcpy(canFrame.data, message, messageLengthToBeProcessed);
	canFrame.can_id = cobID;
	if (rtr) {
		canFrame.can_id |= CAN_RTR_FLAG;
	}
	ssize_t numberOfWrittenBytes = write(m_sock, &canFrame, sizeof(struct can_frame));
	MLOGSOCK(TRC,this) << "write(): " << canFrameToString(canFrame) << " bytes written=" << numberOfWrittenBytes;
	m_statistics.setTimeSinceTransmitted();
	if (( numberOfWrittenBytes < 0) ||	(numberOfWrittenBytes < (int)sizeof(struct can_frame))) { /* ERROR */
		MLOGSOCK(ERR,this) << "While write() :" << CanModuleerrnoToString();
		if ( errno == ENOBUFS ) {
			// std::cerr << "ENOBUFS; waiting a jiffy [100ms]..." << std::endl;
			MLOGSOCK(ERR,this) << "write error ENOBUFS: waiting a jiffy [100ms]...";
			{
				struct timespec tim, tim2;
				tim.tv_sec = 0;
				tim.tv_nsec = 100000;
				if(nanosleep(&tim , &tim2) < 0 ) {
					MLOGSOCK(ERR,this) << "Waiting 100ms failed (nanosleep)";
				}
			}
			ret = false;
		}
		if ( numberOfWrittenBytes < (int)sizeof(struct can_frame)){
			MLOGSOCK(ERR,this) << "Error: Incorrect number of bytes [" << numberOfWrittenBytes << "] written when sending a message.";
			ret = false;
		}

		// trigger the reconnection thread once since there was a send fail. If this repeats N times successively, the
		// m_triggerCounter == 0 and the reconnection thread will do sth.
		MLOGSOCK(WRN,this) << "sendMessage fail detected ( bytes [" << numberOfWrittenBytes
				<< "] written). Trigger reconnection thread and return (no block).";

		decreaseSendFailedCountdown();
		triggerReconnectionThread();
		// return immediately, non blocking
		ret = false;
	} else {
		// no error
		m_statistics.onTransmit( canFrame.can_dlc );
		m_statistics.setTimeSinceTransmitted();

		// reset
		resetSendFailedCountdown();
		// m_failedSendCountdown = m_maxFailedSendCount;
	}
	return ( ret );
}

/**
 * Method that sends a remote request trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
 * @param cobID: Identifier that will be used for the request.
 * @return: Was the initialisation process successful?
 *
 * that is not tested or used until now, and I am not sure it works
 */
bool CSockCanScan::sendRemoteRequest(short cobID)
{
	struct can_frame canFrame;
	int numberOfWrittenBytes;
	canFrame.can_id = cobID + CAN_RTR_FLAG	;
	canFrame.can_dlc = 0;
	numberOfWrittenBytes = write(m_sock, &canFrame, sizeof(struct can_frame));
	do   //TODO: dirty solution, find somethin better (Recursive call?)
	{
		if (numberOfWrittenBytes < 0)
		{
			std::cerr << "Error while calling write() on socket " << CanModuleerrnoToString().c_str() << std::endl;
			if (errno == ENOBUFS)
			{
				MLOGSOCK(ERR,this) << "ENOBUFS; waiting a jiffy [100ms]...";
				{
					struct timespec tim, tim2;
					tim.tv_sec = 0;
					tim.tv_nsec = 100000;
					if(nanosleep(&tim , &tim2) < 0 ) {
						MLOGSOCK(ERR,this) << "Waiting 100ms failed (nanosleep)";
					}
				}
				continue;//If this happens we sleep and start from the beggining of the loop
			}
		}
		else
		{
			if (numberOfWrittenBytes < (int)sizeof(struct can_frame))
			{
				MLOGSOCK(ERR,this) << "sendRemoteRequest() sent less bytes than expected";
				return false;
			}
			m_statistics.setTimeSinceReceived();
		}
		break;//If everything was successful we quit the loop.
	}
	while(true); // thread
	return true;
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
int CSockCanScan::createBus(const string name, const string parameters)
{

	LogItInstance* logItInstance = CCanAccess::getLogItInstance();
	if ( !LogItInstance::setInstance(logItInstance))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not set LogIt instance" << std::endl;

	if (!logItInstance->getComponentHandle(CanModule::LogItComponentName, m_logItHandleSock))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not get LogIt component handle for " << LogItComponentName << std::endl;

	// protect against creating the same bus twice
	bool skip = false;
	{

		std::lock_guard<std::mutex> busMapScopedLock( sockReconnectMutex );

		std::map<string, string>::iterator it = CSockCanScan::m_busMap.find( name );
		if (it == CSockCanScan::m_busMap.end()) {
			CSockCanScan::m_busMap.insert ( std::pair<string, string>(name, parameters) );
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
	m_sock = configureCanBoard(name,parameters);
	if (m_sock < 0) {
		MLOGSOCK(ERR,this) << "Could not create bus [" << name << "] with parameters [" << parameters << "]";
		// take it out from the map again
		std::map<string, string>::iterator it = CSockCanScan::m_busMap.find( name );
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
	m_hCanScanThread = new std::thread( &CSockCanScan::CanScanControlThread, this);
	MLOGSOCK(TRC,this) << "created main thread m_idCanScanThread";

	/**
	 * start a reconnection thread
	 */
	m_hCanReconnectionThread = new std::thread( &CSockCanScan::CanReconnectionThread, this);


	return( 0 );
}

/**
 * Transforms an error frame into an error message (string format):
 * Provides textual representation of SocketCAN error.
 */
std::string CSockCanScan::errorFrameToString(const struct can_frame &canFrame)
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

void CSockCanScan::clearErrorMessage()
{
	string errorMessage = "";
	timeval c_time;
	int ioctlReturn = ioctl(m_sock, SIOCGSTAMP, &c_time);//TODO: Return code is not checked
	if ( ioctlReturn ){
		MLOGSOCK(ERR, this) << "SocketCAN "
				<< getBusName()
				<< " ioctl timestamp from socket failed, setting local time"
				<< " ioctlReturn = " << ioctlReturn;
		c_time = convertTimepointToTimeval( std::chrono::system_clock::now());
	}
	MLOGSOCK(TRC,this) << "ioctlReturn= " << ioctlReturn;
	canMessageError(0, errorMessage.c_str(), c_time);
}

/**
 * send a timestamped error message, get time from the socket or from chrono
 */
void CSockCanScan::sendErrorMessage(const char *mess)
{
	timeval c_time;
	int ioctlReturn = ioctl(m_sock,SIOCGSTAMP,&c_time);//TODO: Return code is not checked
	if ( ioctlReturn ){
		MLOGSOCK(ERR, this) << "SocketCAN "
				<< getBusName()
				<< " ioctl timestamp from socket failed, setting local time"
				<< " ioctlReturn = " << ioctlReturn;
		c_time = convertTimepointToTimeval( std::chrono::system_clock::now());
	}
	MLOGSOCK(TRC,this) << "ioctlReturn= " << ioctlReturn;
	canMessageError(-1,mess,c_time);
}

/**
 * notify the main thread to finish and delete the bus from the map of connections
 */
void CSockCanScan::stopBus ()
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

		std::map<string, string>::iterator it = CSockCanScan::m_busMap.find( m_busName );
		if (it != CSockCanScan::m_busMap.end()) {
			m_hCanScanThread->join();
			CSockCanScan::m_busMap.erase ( it );
			m_busName = "nobus";
		} else {
			MLOGSOCK(DBG,this) << "bus not found in map, is already closed & thread finished, skipping";
		}
	}
}

void CSockCanScan::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_CanParameters.m_lBaudRate);
//	m_statistics.encodeCanModuleStatus();

	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}

/**
 * Report an error when opening a can port
 */
void CSockCanScan::updateInitialError ()
{
	if (m_errorCode == 0) {
		clearErrorMessage();
	} else {
		canMessageError( m_errorCode, "Initial port state: error", convertTimepointToTimeval( std::chrono::system_clock::now()) );
	}
}



