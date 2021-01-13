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
boost::mutex sockReconnectMutex; // protect m_busMap

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
							m_hCanScanThread( NULL ),
							//m_idCanScanThread(std::thread::id()),
							m_errorCode(-1),
							m_busName("nobus"),
							m_logItHandleSock(0)
{
	m_statistics.setTimeSinceOpened();
	m_statistics.beginNewRun();
	m_triggerCounter = m_failedSendCounter;
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
 * reading from socket, and supervising thread for monitoring the sockets/CAN buses.
 * It takes an object reference (cast) and listens with a
 * select call on that socket/object. The select runs with 1Hz, and if
 * there is nothing to receive it should timeout.
 */
/* static */ void CSockCanScan::CanScanControlThread(void *p_voidSockCanScan)
{
	//Message given by the socket.
	struct can_frame  socketMessage;
	CSockCanScan *p_sockCanScan = static_cast<CSockCanScan *>(p_voidSockCanScan);
	std::thread::id _tid = this_thread::get_id();

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
	int statusCountdown = 10;
	while ( p_sockCanScan->m_CanScanThreadRunEnableFlag ) {
		fd_set set;
		FD_ZERO( &set );
		FD_SET( sock, &set );

		// lets update the status every 10th reception, or every 10 seconds, whichever comes earlier
		if ( --statusCountdown <= 0 ){
			p_sockCanScan->updateBusStatus();
			statusCountdown = 10;
		}

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
					timeval t;
					gettimeofday( &t, 0 );
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
			if (numberOfReadBytes < 0) {
				MLOGSOCK(ERR,p_sockCanScan) << "read() error: " << CanModuleerrnoToString()<< " tid= " << _tid;;
				timeval now;
				gettimeofday( &now, 0 );
				p_sockCanScan->canMessageError( numberOfReadBytes, ("read() error: "+CanModuleerrnoToString()).c_str(), now );
				p_sockCanScan->m_errorCode = -1;


				// try close/opening on faults while port is active
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
							// Stopping bus.";
							// p_sockCanScan->stopBus();
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
						} //else {
						// MLOGSOCK(INF,p_sockCanScan) << " tid= " << _tid << "Port reopened.";
						//}

						// the reception handler is still connected to it's signal, by the calling task

					} //else {
					// MLOGSOCK(INF,p_sockCanScan) << " tid= " << _tid << "Leaving Port closed, not needed any more.";
					//}
				} // do...while ... we still have an error
				while ( p_sockCanScan->m_CanScanThreadRunEnableFlag && sock < 0 );


				/**
				 * if we have an open socket again, and we can read numberOfReadBytes >=0 we
				 * should be fine again and continue the thread execution normally
				 */
				continue;
			}


			if (numberOfReadBytes <(int) sizeof(struct can_frame)) {
				MLOGSOCK( WRN, p_sockCanScan ) << p_sockCanScan->m_channelName.c_str() << " incomplete frame received, numberOfReadBytes=[" << numberOfReadBytes << "]";

				// we just report the error and continue the thread normally
				continue;
			}

			if (socketMessage.can_id & CAN_ERR_FLAG) {

				/* With this mechanism we only set the portError */
				p_sockCanScan->m_errorCode = socketMessage.can_id & ~CAN_ERR_FLAG;
				std::string description = errorFrameToString( socketMessage );
				timeval c_time;
				int ioctlReturn1 = ioctl(sock, SIOCGSTAMP, &c_time); //TODO: Return code is not even checked
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

			// remote flag: ignore frame
			if (canMessage.c_rtr) {
				MLOGSOCK(TRC, p_sockCanScan) << " Got a remote CAN message, skipping"<< " tid= " << _tid;
				continue;
			}

			/**
			 * reformat and buffer the message from the socket.
			 * this actually should exclude more bits
			 */
			canMessage.c_id = socketMessage.can_id & ~CAN_RTR_FLAG;
			int ioctlReturn2 = ioctl(sock,SIOCGSTAMP,&canMessage.c_time);   //TODO: Return code is not even checked, yeah, but...
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
			 * the select got nothing to read, this was just a timeout.
			 */
			MLOGSOCK(DBG,p_sockCanScan) << "listening on " << p_sockCanScan->getBusName()
									<< " socket= " << p_sockCanScan->m_sock << " (got nothing)"<< " tid= " << _tid;

			/**
			 * lets check the timeoutOnReception reconnect condition. If it is true, all we can do is to
			 * close/open the socket again since the underlying hardware is hidden by socketcan abstraction.
			 * Like his we do not have to pollute the "sendMessage" like for anagate, and that is cleaner.
			 */
			CanModule::ReconnectAutoCondition rcond = p_sockCanScan->getReconnectCondition();
			CanModule::ReconnectAction ract = p_sockCanScan->getReconnectAction();
			if ( rcond == CanModule::ReconnectAutoCondition::timeoutOnReception && p_sockCanScan->hasTimeoutOnReception()) {
				if ( ract == CanModule::ReconnectAction::singleBus ){
					MLOGSOCK(INF, p_sockCanScan) << " reconnect condition " << (int) rcond
							<< p_sockCanScan->reconnectConditionString(rcond)
							<< " triggered action " << (int) ract
							<< p_sockCanScan->reconnectActionString(ract);
					p_sockCanScan->resetTimeoutOnReception();  // renew timeout while reconnect is in progress
					close( sock );
					sock = p_sockCanScan->openCanPort();
					MLOGSOCK(TRC, p_sockCanScan) << "reconnect one CAN port  sock= " << sock;
				} else {
					MLOGSOCK(INF, p_sockCanScan) << "reconnect action " << (int) ract
							<< p_sockCanScan->reconnectActionString(ract)
							<< " is not implemented for sock";
				}
			}  // reconnect condition
		} // select showed timeout
	} // while ( p_sockCanScan->m_CanScanThreadRunEnableFlag )
	MLOGSOCK(INF,p_sockCanScan) << "main loop of SockCanScan terminated." << " tid= " << _tid;
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
 * update the CAN bus status IFLA from socketcan and make up a CanModule
 * status bitpattern out from this. Each vendor has to implement this with the same
 * bit meanings.
 *
 * Errors are a subset of status.
 *
 * std::string CSockCanScan::errorFrameToString(const struct can_frame &canFrame)
 * fabricates a string, we need a bitpattern
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
void CSockCanScan::updateBusStatus(){
	int ret = 0;

	/* see libsocketcan/include/can_netlink.h
	 *
	 * CAN operational and error states
	 *
	enum can_state {
		CAN_STATE_ERROR_ACTIVE = 0,	 RX/TX error count < 96
		CAN_STATE_ERROR_WARNING,	 RX/TX error count < 128
		CAN_STATE_ERROR_PASSIVE,	 RX/TX error count < 256
		CAN_STATE_BUS_OFF,		 RX/TX error count >= 256
		CAN_STATE_STOPPED,		 Device is stopped
		CAN_STATE_SLEEPING,		 Device is sleeping
		CAN_STATE_MAX
	};
	 */
	int _state;
	ret += can_get_state( m_channelName.c_str(), &_state );

#if 0
	uint32_t _restart_ms;
	struct can_bittiming _bt;
	struct can_ctrlmode _cm;
	struct can_clock _clock;
	struct can_bittiming_const _btc;
	struct can_berr_counter _bc;
	struct can_device_stats _cds;

	ret += can_get_restart_ms( m_channelName.c_str(), &_restart_ms );
	ret += can_get_bittiming( m_channelName.c_str(), &_bt);
	ret += can_get_ctrlmode( m_channelName.c_str(), &_cm);
	ret += can_get_clock( m_channelName.c_str(), &_clock);
	ret += can_get_bittiming_const( m_channelName.c_str(), &_btc);
	ret += can_get_berr_counter( m_channelName.c_str(), &_bc);
	ret += can_get_device_stats( m_channelName.c_str(), &_cds);
#endif

	if ( !ret ) {
		m_statistics.setPortStatus( _state );
	}
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
 */
bool CSockCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
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

		// check the reconnect condition
		switch( m_reconnectCondition ){
		case CanModule::ReconnectAutoCondition::sendFail: {
			MLOGSOCK(WRN, this) << " detected a sendFail, triggerCounter= " << m_triggerCounter
					<< " failedSendCounter= " << m_failedSendCounter;
			m_triggerCounter--;
			break;
		}
		case CanModule::ReconnectAutoCondition::never:
		default:{
			m_triggerCounter = m_failedSendCounter;
			break;
		}
		}

		// do the reconnect action if triggerCounter says so
		switch ( m_reconnectAction ){
		case CanModule::ReconnectAction::singleBus: {
			if ( m_triggerCounter <= 0 ){
				MLOGSOCK(INF, this) << " reconnect condition " << (int) m_reconnectCondition
						<< reconnectConditionString(m_reconnectCondition)
						<< " triggered action " << (int) m_reconnectAction
						<< reconnectActionString(m_reconnectAction);
				close( m_sock );
				MLOGSOCK(TRC, this) << "calling openCanPort() for " << this->getBusName();
				int return0 = openCanPort();
				MLOGSOCK(TRC, this) << "reconnect one CAN port  ret= " << return0;
				m_triggerCounter = m_failedSendCounter;
				MLOGSOCK(TRC, this) << "set internal triggerCounter= " << m_triggerCounter;
				{
					MLOGSOCK(WRN,this) << "write error ENOBUFS: waiting a jiffy [100ms]...";
					struct timespec tim, tim2;
					tim.tv_sec = 0;
					tim.tv_nsec = 100000;
					if(nanosleep(&tim , &tim2) < 0 ) {
						MLOGSOCK(ERR,this) << "Waiting 100ms failed (nanosleep)";
					}
				}
			}
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
			 * to unplug the USB. Therefore "allBusesOnBridge" as reconnect action is not available.
			 */
			MLOGSOCK(WRN, this) << "reconnection action "
					<< (int) m_reconnectAction << reconnectActionString( m_reconnectAction )
					<< " is not available for the socketcan/linux implementation.";
			break;
		}
		} // switch
	} else {
		// no error
		m_statistics.onTransmit( canFrame.can_dlc );
		m_statistics.setTimeSinceTransmitted();
		m_triggerCounter = m_failedSendCounter;
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

void testThread(){
	while(true){
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << std::endl;

		struct timespec tim, tim2;
		tim.tv_sec = 1;
		tim.tv_nsec = 0;
		if(nanosleep(&tim , &tim2) < 0 ) {
			std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << "nanosleep failed" << std::endl;;
		}
	}
}
void testThread2(void *p_voidSockCanScan){
	while(true){
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << std::endl;

		struct timespec tim, tim2;
		tim.tv_sec = 1;
		tim.tv_nsec = 0;
		if(nanosleep(&tim , &tim2) < 0 ) {
			std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << "nanosleep failed" << std::endl;;
		}
	}
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
		sockReconnectMutex.lock();
		std::map<string, string>::iterator it = CSockCanScan::m_busMap.find( name );
		if (it == CSockCanScan::m_busMap.end()) {
			CSockCanScan::m_busMap.insert ( std::pair<string, string>(name, parameters) );
			m_busName = name;
		} else {
			skip = true;
		}
		sockReconnectMutex.unlock();
	}
	if ( skip ){
		LOG(Log::WRN) << __FUNCTION__ << " bus exists already [" << name << ", " << parameters << "], skipping board config";
		return( 1 );
	}

	m_CanScanThreadRunEnableFlag = true;
	m_sock = configureCanBoard(name,parameters);
	if (m_sock < 0) {
		MLOGSOCK(ERR,this) << "Could not create bus [" << name << "] with parameters [" << parameters << "]";
		return -1;
	}
	MLOGSOCK(TRC,this) << "Created bus with parameters [" << parameters << "], starting main loop";
	std::thread zz( &CSockCanScan::CanScanControlThread, (void *) this );
	m_hCanScanThread = &zz;
	MLOGSOCK(TRC,this) << "created main thread m_idCanScanThread= " << zz.get_id();
	return( 0 );
}

/*
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
	MLOGSOCK(TRC,this) << "ioctlReturn= " << ioctlReturn;
	canMessageError(0, errorMessage.c_str(), c_time);
}

void CSockCanScan::sendErrorMessage(const char *mess)
{
	timeval c_time;
	int ioctlReturn = ioctl(m_sock,SIOCGSTAMP,&c_time);//TODO: Return code is not checked
	MLOGSOCK(TRC,this) << "ioctlReturn= " << ioctlReturn;
	canMessageError(-1,mess,c_time);
}

/**
 * notify the main thread to finish and delete the bus from the map of connections
 */
bool CSockCanScan::stopBus ()
{
	MLOGSOCK(DBG,this) << __FUNCTION__ << " m_busName= " <<  m_busName;
	// notify the thread that it should finish.
	m_CanScanThreadRunEnableFlag = false;
	MLOGSOCK(DBG,this) << " try joining threads...";

	{
		sockReconnectMutex.lock();
		std::map<string, string>::iterator it = CSockCanScan::m_busMap.find( m_busName );
		if (it != CSockCanScan::m_busMap.end()) {
			m_hCanScanThread->join();
			CSockCanScan::m_busMap.erase ( it );
			m_busName = "nobus";
		} else {
			MLOGSOCK(DBG,this) << " not joining threads... bus does not exist";
		}
		sockReconnectMutex.unlock();
	}
	MLOGSOCK(DBG,this) << "stopBus() finished";
	return true;
}

void CSockCanScan::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_CanParameters.m_lBaudRate);
	m_statistics.encodeCanModuleStatus();

	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}

void CSockCanScan::updateInitialError ()
{
	if (m_errorCode == 0) {
		clearErrorMessage();
	} else {
		timeval now;
		gettimeofday( &now, 0);
		canMessageError( m_errorCode, "Initial port state: error", now );
	}
}



