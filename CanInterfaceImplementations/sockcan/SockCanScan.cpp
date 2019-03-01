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
#include <errno.h>
#include <fstream>
#include <iostream>

#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>

#include <CanModuleUtils.h>

#include <LogIt.h>
using namespace std;

/* static */ std::map<string, string> CSockCanScan::m_busMap = {{"dummy_name", "dummy_parameters"}};



//! The macro below is applicable only to this translation unit
//#define MLOG(LEVEL,THIS) LOG(Log::LEVEL) << THIS->m_channelName << " "

//This function creates an instance of this class and returns it. It will be loaded trough the dll to access the rest of the code.
extern "C" CCanAccess *getCanBusAccess()
{
	LOG(Log::TRC) << __FUNCTION__ << " " __FILE__ << " " << __LINE__;
	CCanAccess *cc = new CSockCanScan();
	return cc;
}

CSockCanScan::CSockCanScan() :
			m_CanScanThreadShutdownFlag(false),
			m_sock(0),
			m_hCanScanThread(0),
			m_idCanScanThread(0),
			m_errorCode(-1)
{
	m_statistics.beginNewRun();
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
void* CSockCanScan::CanScanControlThread(void *p_voidSockCanScan)
{
	//Message given by the socket.
	struct can_frame  socketMessage;
	CSockCanScan *p_sockCanScan = static_cast<CSockCanScan *>(p_voidSockCanScan);
	p_sockCanScan->m_CanScanThreadShutdownFlag = true;
	int sock = p_sockCanScan->m_sock;

	{
		// discard first read
		fd_set set;
		FD_ZERO( &set );
		FD_SET( sock, &set );

		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int selectResult = select( sock+1, &set, 0, &set, &timeout );
		if ( selectResult > 0 ) {
			int numberOfReadBytes = read(sock, &socketMessage, sizeof(can_frame));
			MLOG(INF,p_sockCanScan) << "discarding first read(): " << canFrameToString(socketMessage)
					<< "got numberOfReadBytes= " << numberOfReadBytes << " discard them";
		}
	}

	MLOG(TRC,p_sockCanScan) << "Main loop of SockCanScan starting";
	while ( p_sockCanScan->m_CanScanThreadShutdownFlag ) {
		fd_set set;
		FD_ZERO( &set );
		FD_SET( sock, &set );

		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int selectResult = select( sock+1, &set, 0, &set, &timeout );

		/**
		 * the select failed. that is very bad, system problem, but let's continue nevertheless.
		 * There is not much we can do
		 */
		if ( selectResult < 0 ){
			MLOG(ERR,p_sockCanScan) << "select() failed: " << CanModuleerrnoToString();
			{
				struct timespec tim, tim2;
				tim.tv_sec = 1;
				tim.tv_nsec = 0;
				if(nanosleep(&tim , &tim2) < 0 ) {
					MLOG(ERR,p_sockCanScan) << "Waiting 1s failed (nanosleep)";
				}
			}
			//	usleep (1000000);
			continue;
		}


		/**
		 * select returned, either on the timeout or with something on that socket/object.
		 *  Assumption: at this moment sock holds meaningful value.
		 * Now select result >=0 so it was either nothing received (timeout) or something received
		 */

		if (p_sockCanScan->m_errorCode) {
			/** The preceding call took either 'timeout' time, or there is frame received --
			 * perfect time to attempt to clean error frame.
			 */
			int newErrorCode = -1;
			int ret = can_get_state( p_sockCanScan->m_channelName.c_str(), &newErrorCode);
			if (ret != 0) {
				MLOG(ERR,p_sockCanScan) << "can_get_state failed.";
			} else {
				if (newErrorCode == 0) {
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
			MLOG(DBG,p_sockCanScan) << "read(): " << canFrameToString(socketMessage);
			MLOG(DBG,p_sockCanScan) << "got numberOfReadBytes= " << numberOfReadBytes;

			if (numberOfReadBytes < 0) {
				MLOG(ERR,p_sockCanScan) << "read() error: " << CanModuleerrnoToString();
				timeval now;
				gettimeofday( &now, 0 );
				p_sockCanScan->canMessageError( numberOfReadBytes, ("read() error: "+CanModuleerrnoToString()).c_str(), now );
				p_sockCanScan->m_errorCode = -1;


				// try close/opening on faults while port is active
				do {
					// MLOG(INF,p_sockCanScan) << "Waiting " << seconds << " seconds.";
					MLOG(INF,p_sockCanScan) << "Waiting 10000ms.";
					{
						struct timespec tim, tim2;
						tim.tv_sec = 10;
						tim.tv_nsec = 0;
						if(nanosleep(&tim , &tim2) < 0 ) {
							MLOG(ERR,p_sockCanScan) << "Waiting 10000ms failed (nanosleep)";
						}
					}
					if ( sock > 0 )	{
						// try closing the socket
						MLOG(INF,p_sockCanScan) << "Closing socket.";
						if (close(sock) < 0) {
							MLOG(ERR,p_sockCanScan) << "Socket close error!";
							// Stopping bus.";
							// p_sockCanScan->stopBus();
						} else {
							MLOG(INF,p_sockCanScan) << "Socket closed";
							sock = -1;
						}
					}

					/**
					 * try to re-open the socket.
					 * only reopen the socket if we still need it.
					 * If we need to shutdown the thread, leave it closed and
					 * terminate main loop.
					 */
					if ( p_sockCanScan->m_CanScanThreadShutdownFlag ) {
						MLOG(INF,p_sockCanScan) << "Now port will be reopened.";
						if ((sock = p_sockCanScan->openCanPort()) < 0) {
							MLOG(ERR,p_sockCanScan) << "openCanPort() failed.";
						} else {
							MLOG(INF,p_sockCanScan) << "Port reopened.";
						}
					} else {
						MLOG(INF,p_sockCanScan) << "Leaving Port closed, not needed any more.";
					}
				} // do...while ... we still have an error
				while ( p_sockCanScan->m_CanScanThreadShutdownFlag && sock < 0 );


				/**
				 * if we have an open socket again, and we can read numberOfReadBytes >=0 we
				 * should be fine again and continue the thread execution normally
				 */
				continue;
			}


			if (numberOfReadBytes <(int) sizeof(struct can_frame)) {
				LOG(Log::INF) << p_sockCanScan->m_channelName.c_str() << " incomplete frame received, numberOfReadBytes=[" << numberOfReadBytes << "]";

				// we just report the error and continue the thread normally
				continue;
			}

			if (socketMessage.can_id & CAN_ERR_FLAG) {

				/* With this mechanism we only set the portError */
				p_sockCanScan->m_errorCode = socketMessage.can_id & ~CAN_ERR_FLAG;
				std::string description = errorFrameToString( socketMessage );
				timeval c_time;
				int ioctlReturn1 = ioctl(sock, SIOCGSTAMP, &c_time); //TODO: Return code is not even checked
				MLOG(ERR, p_sockCanScan) << "SocketCAN ioctl return: [" << ioctlReturn1
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
				MLOG(TRC, p_sockCanScan) << __FUNCTION__ << " Got a remote CAN message, skipping";
				continue;
			}

			/**
			 * reformat and buffer the message from the socket.
			 * this actually should exclude more bits
			 */
			canMessage.c_id = socketMessage.can_id & ~CAN_RTR_FLAG;
			int ioctlReturn2 = ioctl(sock,SIOCGSTAMP,&canMessage.c_time); //TODO: Return code is not even checked
			MLOG(TRC, p_sockCanScan) << __FUNCTION__ << " SocketCAN ioctl return: [" << ioctlReturn2 << "]";
			canMessage.c_dlc = socketMessage.can_dlc;
			memcpy(&canMessage.c_data[0],&socketMessage.data[0],8);
			p_sockCanScan->canMessageCame( canMessage );
			p_sockCanScan->m_statistics.onReceive( socketMessage.can_dlc );
		} else {
			/**
			 * the select got nothing to read, this was just a timeout.
			 */
			MLOG(DBG,p_sockCanScan) << "listening on " << p_sockCanScan->getBusName()
					<< " socket= " << p_sockCanScan->m_sock << " (got nothing)";
		}
	}
	MLOG(INF,p_sockCanScan) << "Main loop of SockCanScan terminated." ;
	pthread_exit(NULL);
}

CSockCanScan::~CSockCanScan()
{
	stopBus();
}

int CSockCanScan::configureCanBoard(const string name,const string parameters)
{
	vector<string> parset;
	parset = parseNameAndParameters( name, parameters );
	std::ostringstream channelName;
	channelName << "can" << parset[1]; // we need "can" prefixed for socketcan, but the user should not care
	m_channelName = channelName.str();
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
			MLOG(ERR,this) << "Error while can_do_stop(): " << CanModuleerrnoToString();
			return -1;
		}
		returnCode = can_set_bitrate(m_channelName.c_str(),m_CanParameters.m_lBaudRate);
		if (returnCode < 0)
		{
			MLOG(ERR,this) << "Error while can_set_bitrate(): " << CanModuleerrnoToString();
			return -1;
		}
		returnCode = can_do_start(m_channelName.c_str());
		if (returnCode < 0)
		{
			MLOG(ERR,this) << "Error while can_do_start(): " << CanModuleerrnoToString();
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
		MLOG(ERR,this) << "Given name of the port exceeds operating-system imposed limit";
		return -1;
	}
	strncpy(ifr.ifr_name, m_channelName.c_str(), sizeof(ifr.ifr_name)-1);

	if (ioctl(m_sock, SIOCGIFINDEX, &ifr) < 0)
	{
		MLOG(ERR,this) << "ioctl SIOCGIFINDEX failed. " << CanModuleerrnoToString();
		return -1;
	}

	can_err_mask_t err_mask = 0x1ff;
	if( setsockopt(m_sock, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof err_mask) < 0 )
	{
		MLOG(ERR,this) << "setsockopt() failed: " << CanModuleerrnoToString();
		return -1;
	}

	struct sockaddr_can socketAdress;
	socketAdress.can_family = AF_CAN;
	socketAdress.can_ifindex = ifr.ifr_ifindex;

	if (::bind(m_sock, (struct sockaddr *)&socketAdress, sizeof(socketAdress)) < 0)
	{
		MLOG(ERR,this) << "While bind(): " << CanModuleerrnoToString();
		return -1;
	}

	// Fetch initial state of the port
	if (int ret=can_get_state( m_channelName.c_str(), &m_errorCode ))
	{
		m_errorCode = ret;
		MLOG(ERR,this) << "can_get_state() failed with error code " << ret << ", it was not possible to obtain initial port state";
	}

	updateInitialError();

	// systec: we have no reception handler connected

	return m_sock;
}



bool CSockCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	int messageLengthToBeProcessed;

	struct can_frame canFrame = CSockCanScan::emptyCanFrame();

	if (len > 8)
	{
		messageLengthToBeProcessed = 8;
		MLOG(DBG,this) << "The Length is more then 8 bytes: " << std::dec << len;
	}
	else
	{
		messageLengthToBeProcessed = len;
	}
	canFrame.can_dlc = messageLengthToBeProcessed;
	memcpy(canFrame.data, message, messageLengthToBeProcessed);
	canFrame.can_id = cobID;
	if (rtr)
	{
		canFrame.can_id |= CAN_RTR_FLAG;
	}

	ssize_t numberOfWrittenBytes = write(m_sock, &canFrame, sizeof(struct can_frame));

	MLOG(TRC,this) << "write(): " << canFrameToString(canFrame) << " bytes written=" << numberOfWrittenBytes;

	if (numberOfWrittenBytes < 0) /* ERROR */
	{
		MLOG(ERR,this) << "While write() :" << CanModuleerrnoToString();
		if (errno == ENOBUFS)
		{
			std::cerr << "ENOBUFS; waiting a jiffy [100ms]..." << std::endl;
			MLOG(ERR,this) << "ENOBUFS; waiting a jiffy [100ms]...";
			usleep(100000);
		}
	}

	if (numberOfWrittenBytes > 0)
	{
		m_statistics.onTransmit( canFrame.can_dlc );
	}
	if (numberOfWrittenBytes < (int)sizeof(struct can_frame))
	{
		std::cerr << "Error: Incorrect number of bytes [" << numberOfWrittenBytes << "] written when sending a message." << std::endl;
		MLOG(ERR,this) << "Error: Incorrect number of bytes [" << numberOfWrittenBytes << "] written when sending a message.";
		return false;
	}

	return true;
}

bool CSockCanScan::sendRemoteRequest(short cobID)
{
	struct can_frame canFrame;
	int numberOfWrittenBytes;
	canFrame.can_id = cobID + CAN_RTR_FLAG	;
	canFrame.can_dlc = 0;
	numberOfWrittenBytes = write(m_sock, &canFrame, sizeof(struct can_frame));
	do//TODO: dirty solution, find somethin better (Recursive call?)
	{
		if (numberOfWrittenBytes < 0)
		{
			std::cerr << "Error while calling write() on socket " << CanModuleerrnoToString().c_str() << std::endl;
			if (errno == ENOBUFS)
			{
				std::cout << "ENOBUFS; waiting a jiffy ..." << std::endl;
				// insert deleted systec buffer
				usleep(100000);
				continue;//If this happens we sleep and start from the beggining of the loop
			}
		}
		else
		{
			if (numberOfWrittenBytes < (int)sizeof(struct can_frame))
			{
				std::cout << "Error: sendRemoteRequest() sent less bytes than expected" << std::endl;
				return false;
			}
		}
		break;//If everything was successful we quit the loop.
	}
	while(true);
	return true;
}

bool CSockCanScan::createBus(const string name, const string parameters)
{
#if 0
	bool skip = false;
	std::map<string, string>::iterator it = CSockCanScan::m_busMap.find( name );
	if (it == CSockCanScan::m_busMap.end()) {
		CSockCanScan::m_busMap.insert ( std::pair<string, string>(name, parameters) );
	} else {
		LOG(Log::WRN) << __FUNCTION__ << " bus exists already [" << name << ", " << parameters << "], skipping thread";
		skip = true;
		// return(	true ); // thread exists already
	}
#endif
	m_CanScanThreadShutdownFlag = true;

	LOG(Log::TRC) << __FUNCTION__ << " Creating bus [" << name << "] with parameters [" << parameters << "]";

	m_sock = configureCanBoard(name,parameters);
	if (m_sock < 0) {
		LOG(Log::ERR) << "Could not create bus [" << name << "] with parameters [" << parameters << "]";
		return false;
	}
	//if ( !skip ){
		LOG(Log::TRC) << __FUNCTION__ << " OK: Created bus [" << name << "] with parameters [" << parameters << "], starting main loop";
		m_idCanScanThread =	pthread_create(&m_hCanScanThread,NULL,&CanScanControlThread, (void *)this);
		return (!m_idCanScanThread);
	//} else {
	//	m_idCanScanThread = 0;
	//	return( true );
	//}
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
	MLOG(TRC,this) << __FUNCTION__ << " ioctlReturn= " << ioctlReturn;
	canMessageError(0, errorMessage.c_str(), c_time);
}

void CSockCanScan::sendErrorMessage(const char *mess)
{
	timeval c_time;
	int ioctlReturn = ioctl(m_sock,SIOCGSTAMP,&c_time);//TODO: Return code is not checked
	MLOG(TRC,this) << __FUNCTION__ << " ioctlReturn= " << ioctlReturn;
	canMessageError(-1,mess,c_time);
}

bool CSockCanScan::stopBus ()
{
	// notify the thread that it should finish.
	m_CanScanThreadShutdownFlag = false;
	MLOG(INF,this) << " joining threads...";
	pthread_join( m_hCanScanThread, 0 );
	MLOG(INF,this) << "stopBus() finished";
	return true;
}

void CSockCanScan::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_CanParameters.m_lBaudRate);
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}

void CSockCanScan::updateInitialError ()
{
	if (m_errorCode == 0)
	{
		clearErrorMessage();
	}
	else
	{
		timeval now;
		gettimeofday( &now, 0);
		canMessageError( m_errorCode, "Initial port state: error", now );
	}

}



