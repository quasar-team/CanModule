/*
 * SockCanScan.cpp
 *
 *  Created on: Jul 21, 2011
 *
 *  Based on work by vfilimon
 *  Rework and logging done by Piotr Nikiel <piotr@nikiel.info>
 *
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

//! The macro below is applicable only to this translation unit
//#define MLOG(LEVEL,THIS) LOG(Log::LEVEL) << THIS->m_channelName << " "

//This function creates an instance of this class and returns it. It will be loaded trough the dll to access the rest of the code.
extern "C" CCanAccess *getCanBusAccess()
{
  LOG(Log::TRC) << "in getCanBusAccess ";
  CCanAccess *cc;
  cc = new CSockCanScan();
  return cc;
}

CSockCanScan::CSockCanScan() :
				m_sock(0),
//				m_baudRate(0),
//				m_dontReconfigure (false),
				m_errorCode(-1),
				m_CanScanThreadShutdownFlag(false)
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

void* CSockCanScan::CanScanControlThread(void *p_voidSockCanScan)
{
	//Message given by the socket.
	struct can_frame  socketMessage;
	CSockCanScan *p_sockCanScan = static_cast<CSockCanScan *>(p_voidSockCanScan);
	p_sockCanScan->m_CanScanThreadShutdownFlag = true;
	int sock = p_sockCanScan->m_sock;

	while (p_sockCanScan->m_CanScanThreadShutdownFlag)
	{
		/* Assumption: at this moment sock holds meaningful value. */

		fd_set set;
		FD_ZERO( &set );
		FD_SET( sock, &set );
		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int selectResult = select( sock+1, &set, 0, &set, &timeout );

		if (selectResult < 0)
		{
			MLOG(ERR,p_sockCanScan) << "select() failed: " << CanModuleerrnoToString();
			usleep (1000000);
			continue;

		}
		/* Now select result >=0 so it was either nothing received (timeout) or something received */

		if (p_sockCanScan->m_errorCode)
		{
			/* The preceeding call took either 'timeout' time, or there is frame received -- perfect time to attempt to clean error frame. */
			int newErrorCode = -1;
			int ret = can_get_state( p_sockCanScan->m_channelName.c_str(), &newErrorCode);
			if (ret != 0)
			{
				MLOG(ERR,p_sockCanScan) << "can_get_state failed.";
			}
			else
			{
				if (newErrorCode == 0)
				{
					p_sockCanScan->m_errorCode = 0;
					timeval t;
					gettimeofday( &t, 0 );
					p_sockCanScan->canMessageError( p_sockCanScan->m_errorCode, "CAN port is recovered", t );
				}
			}
		}


		if (selectResult > 0)
		{
			int numberOfReadBytes = read(sock, &socketMessage, sizeof(can_frame));
//			MLOG(DBG,p_sockCanScan) << "read(): " << canFrameToString(socketMessage);
			if (numberOfReadBytes < 0)
			{
				const int seconds = 10;
				MLOG(ERR,p_sockCanScan) << "read() error: " << CanModuleerrnoToString();
				timeval now;
				gettimeofday( &now, 0 );
				p_sockCanScan->canMessageError( numberOfReadBytes, ("read() error: "+CanModuleerrnoToString()).c_str(), now );
				p_sockCanScan->m_errorCode = -1;
				do
				{
					MLOG(INF,p_sockCanScan) << "Waiting " << seconds << " seconds.";
					sleep(seconds);
					if (sock>0)
					{
						MLOG(INF,p_sockCanScan) << "Closing socket";
						if (close(sock) < 0)
						{
							MLOG(ERR,p_sockCanScan) << "Socket close error!";
						}
						else
						{
							MLOG(INF,p_sockCanScan) << "Socket closed";
							sock = -1;
						}
					}
					MLOG(INF,p_sockCanScan) << "Now port will be reopened.";
					if ((sock = p_sockCanScan->openCanPort()) < 0)
					{
						MLOG(ERR,p_sockCanScan) << "openCanPort() failed.";
					}
					else
					{
						MLOG(INF,p_sockCanScan) << "Port reopened.";
					}
				}
				while (p_sockCanScan->m_CanScanThreadShutdownFlag && sock<0);
				continue;
			}

			if (numberOfReadBytes <(int) sizeof(struct can_frame))
			{
				LOG(Log::INF) << p_sockCanScan->m_channelName.c_str() << " incomplete frame received, numberOfReadBytes=[" << numberOfReadBytes << "]";
				continue;
			}
	
			if (socketMessage.can_id & CAN_ERR_FLAG)
			{

				/* With this mechanism we only set the portError */
				p_sockCanScan->m_errorCode = socketMessage.can_id & ~CAN_ERR_FLAG;
				std::string description = errorFrameToString( socketMessage );
				timeval c_time;
				int ioctlReturn1 = ioctl(sock, SIOCGSTAMP, &c_time); //TODO: Return code is not even checked
				MLOG(ERR, p_sockCanScan) << "SocketCAN error frame: [" << description << "], original: [" << canFrameToString(socketMessage) << "]";
				p_sockCanScan->canMessageError( p_sockCanScan->m_errorCode, description.c_str(), c_time );

				continue;
			}

			CanMessage canMessage;
			canMessage.c_rtr = socketMessage.can_id & CAN_RTR_FLAG;
                 if (canMessage.c_rtr) continue;
			// this actually should exclude more bits
			canMessage.c_id = socketMessage.can_id & ~CAN_RTR_FLAG;
			int ioctlReturn2 = ioctl(sock,SIOCGSTAMP,&canMessage.c_time); //TODO: Return code is not even checked
			canMessage.c_dlc = socketMessage.can_dlc;
			memcpy(&canMessage.c_data[0],&socketMessage.data[0],8);
			p_sockCanScan->canMessageCame(canMessage);
			p_sockCanScan->m_statistics.onReceive( socketMessage.can_dlc );
		}
	}

//	MLOG(INF,p_sockCanScan) << "Main loop of SockCanScan terminated." ;
	
	// loop finished, close open socket
	// These is a redundent code
	if (sock) {
//		MLOG(DBG,p_sockCanScan) <<  "Closing socket for " << p_sockCanScan->m_channelName.c_str() << std::endl;
		if (close(sock) < 0) {
			perror("Socket close error!");
		}
		else
		{
			MLOG(DBG,p_sockCanScan) <<  "Closed socket for " << p_sockCanScan->m_channelName.c_str() << std::endl;
		}
		sock = 0;
	}
	pthread_exit(NULL);
}

CSockCanScan::~CSockCanScan()
{
	stopBus();
}

int CSockCanScan::configureCanBoard(const string name,const string parameters)
{
    vector<string> parset;
    parset = parcerNameAndPar(name,parameters);
    m_channelName = parset[1];
    return openCanPort();
}

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

    return m_sock;
}

bool CSockCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
  int messageLengthToBeProcessed;
  
  struct can_frame canFrame;
  if (rtr)
  {
	  canFrame.can_id |= CAN_RTR_FLAG;
  }

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
    canFrame.can_dlc = messageLengthToBeProcessed;

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
    	MLOG(ERR,this) << << "Error: Incorrect number of bytes [" << numberOfWrittenBytes << "] written when sending a message.";
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

bool CSockCanScan::createBus(const string name ,const string parameters)
{
	m_CanScanThreadShutdownFlag = true;
  
	LOG(Log::INF) << "Creating bus [" << name << "] with parameters [" << parameters << "]";

	m_sock = configureCanBoard(name,parameters);
	if (m_sock < 0)
	{
		return false;
	}
	LOG(Log::INF) << "Starting main loop for CAN bus [" << name << "]";

	m_idCanScanThread =	pthread_create(&m_hCanScanThread,NULL,&CanScanControlThread, (void *)this);

	return (!m_idCanScanThread);
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
  canMessageError(0, errorMessage.c_str(), c_time);
}

void CSockCanScan::sendErrorMessage(const char *mess)
{
  timeval c_time;
  int ioctlReturn = ioctl(m_sock,SIOCGSTAMP,&c_time);//TODO: Return code is not checked
  canMessageError(-1,mess,c_time);
}

bool CSockCanScan::stopBus ()
{
	// notify the thread that it should finish.
	m_CanScanThreadShutdownFlag = false;
	pthread_join( m_hCanScanThread, 0 );
//	MLOG(INF,this) << "stopBus() finished";
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



