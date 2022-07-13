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

#include <mutex>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <linux/sockios.h> // needed for SIOCGSTAMP in can-utils kernel since 4.feb.2020 3.10.0-1062.12.1.el7.x86_64

#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>
#include <can_netlink.h> // for can_state
#include <libsocketcan.h> // for can_get_state ...

#include <CanModuleUtils.h>

#include <LogIt.h>

using namespace std;

/* static */ std::map<string, string> CSockCanScan::m_busMap; // TODO: maps what to what ?

std::mutex sockReconnectMutex; // protect m_busMap

#define MLOGSOCK(LEVEL,THIS) LOG(Log::LEVEL, THIS->logItHandle()) << __FUNCTION__ << " sock bus= " << THIS->getBusName() << " "

#define LOG_AND_THROW(EXCEPTION_TYPE, MSG, THIS) \
    { \
    LOG(Log::ERR, THIS->logItHandle()) << MSG; \
    throw EXCEPTION_TYPE (MSG); \
    }

//TODO another file ?
static std::string translateCanStateToText(can_state state)
{
	switch (state)
	{
		case CAN_STATE_ERROR_ACTIVE: return "ERROR_ACTIVE (approximate OK)";
		case CAN_STATE_ERROR_WARNING: return "ERROR_WARNING";
		case CAN_STATE_ERROR_PASSIVE: return "ERROR_PASSIVE";
		case CAN_STATE_BUS_OFF: return "BUS_OFF";
		case CAN_STATE_STOPPED: return "CAN_STATE_STOPPED";
		case CAN_STATE_SLEEPING: return "CAN_STATE_SLEEPING";
		default:
			throw std::logic_error("Unhanndled state condition");
	}
} 

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
							m_sock(-1), // unassigned
							m_errorCode(-1),
							m_hCanScanThread( NULL ),
							m_busName("nobus"),
							m_logItHandleSock(0)
{
	//m_statistics.setTimeSinceOpened(); -- TODO Piotr Uncomment it later
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
void CSockCanScan::backgroundThread()
{
	struct can_frame  socketMessage;

	std::string _tid;
	{
		std::stringstream ss;
		ss << this_thread::get_id();
		_tid = ss.str();
	}
	MLOGSOCK(TRC, this) << "created main loop tid= " << _tid;

	m_CanScanThreadRunEnableFlag = true;
	
	MLOGSOCK(INF, this) << "waiting for first reception on socket " << m_sock;

	int selectResult = selectWrapper();
	if ( selectResult > 0 )
	{
		int numberOfReadBytes = read(m_sock, &socketMessage, sizeof(can_frame));
		MLOGSOCK(INF, this) << "discarding first read on socket " << m_sock
				<< " " << canFrameToString(socketMessage)
				<< "got numberOfReadBytes= " << numberOfReadBytes << " discard them";
	}	
	

	MLOGSOCK(TRC, this) << "main loop of SockCanScan starting, tid= " << _tid;
	while ( m_CanScanThreadRunEnableFlag )
	{
		fetchAndPublishState();

		int selectResult = selectWrapper();

		/**
		 * the select failed. that is very bad, system problem, but let's continue nevertheless.
		 * There is not much we can do
		 */
		if ( selectResult < 0 )
		{
			publishStatus(selectResult, "select() failed: " + CanModuleerrnoToString() + " tid= " + _tid);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}
		if (selectResult == 0)
		{
			MLOGSOCK(DBG, this) << "listening on " << getBusName() << " socket= " << m_sock << " (got nothing)"<< " tid= " << _tid;
			continue;
		}
		/**
		 * select reports that it has got something, so no timeout in this case
		 */

		int numberOfReadBytes = read(m_sock, &socketMessage, sizeof(can_frame));
		MLOGSOCK(DBG, this) << "read(): " << canFrameToString(socketMessage) << " tid= " << _tid;
		MLOGSOCK(DBG, this) << "got numberOfReadBytes= " << numberOfReadBytes << " tid= " << _tid;;
		if (numberOfReadBytes < 0)
		{
			publishStatus(numberOfReadBytes, "read() failed: " + CanModuleerrnoToString() + " tid= " + _tid);
			// try close/opening on faults while port is active

			recoverPort();

			/**
			 * if we have an open socket again, and we can read numberOfReadBytes >=0 we
			 * should be fine again and continue the thread execution normally
			 */
			continue;
		}

		if (numberOfReadBytes <(int) sizeof(struct can_frame)) {
			MLOGSOCK( WRN, this ) << m_channelName << " incomplete frame received, numberOfReadBytes=[" << numberOfReadBytes << "]";

			// we just report the error and continue the thread normally
			continue;
		}

		if (socketMessage.can_id & CAN_ERR_FLAG) {

			/* With this mechanism we only set the portError */
			int code = socketMessage.can_id & ~CAN_ERR_FLAG;
			std::string description = CSockCanScan::errorFrameToString( socketMessage );
			publishStatus( code, description, true);
			// we have tried to get the error, and we continue thread normally
			continue;
		}

		/**
		 * we have a CAN message, error free, lets digest it
		 */
		CanMessage canMessage;
		canMessage.c_rtr = socketMessage.can_id & CAN_RTR_FLAG;


		/**
		 * reformat and buffer the message from the socket.
		 * this actually should exclude more bits
		 */
		canMessage.c_id = socketMessage.can_id & ~CAN_RTR_FLAG;
		int ioctlReturn2 = ioctl(m_sock,SIOCGSTAMP,&canMessage.c_time);   //TODO: Return code is not even checked, yeah, but...
		MLOGSOCK(TRC, this) << " SocketCAN ioctl SIOCGSTAMP return: [" << ioctlReturn2 << "]" << " tid= " << _tid;
		canMessage.c_dlc = socketMessage.can_dlc;
		memcpy(&canMessage.c_data[0],&socketMessage.data[0],8);
		MLOGSOCK(TRC, this) << " sending message id= " << canMessage.c_id << " through signal with payload";
		canMessageCame( canMessage ); // signal with payload to trigger registered handler
		m_statistics.onReceive( socketMessage.can_dlc );

		// we can reset the reconnectionTimeout here, since we have received a message
		//resetTimeoutOnReception(); -- TODO Piotr uncomment it later


	}
	MLOGSOCK(INF, this) << "main loop of SockCanScan terminated." << " tid= " << _tid;
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
 * 
 * @throws
 * 
 * returns the file descriptor.
 */
int CSockCanScan::openCanPort()
{
	/* Determine if it was requested through params to configure the port (baudrate given) or just to open it ("Unspecified")*/

	if (!m_CanParameters.m_dontReconfigure)
	{
		int returnCode = can_do_stop(m_channelName.c_str());
		if (returnCode < 0)
			LOG_AND_THROW(std::runtime_error, "Error while can_do_stop() for bus [" + m_channelName+ "]: " + CanModuleerrnoToString(), this);
		
		returnCode = can_set_bitrate(m_channelName.c_str(), m_CanParameters.m_lBaudRate);
		if (returnCode < 0)
			LOG_AND_THROW(std::runtime_error, "Error while can_set_bitrate() for bus [" + m_channelName+ "]: " + CanModuleerrnoToString(), this);

		returnCode = can_do_start(m_channelName.c_str());
		if (returnCode < 0)
			LOG_AND_THROW(std::runtime_error, "Error while can_do_start() for bus [" + m_channelName+ "]: " + CanModuleerrnoToString(), this);
	}
	else
	{
		MLOGSOCK(INF, this) << "did NOT reconfigure (stop, set bitrate, start) the CAN port";
	}

	m_sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (m_sock < 0)
		LOG_AND_THROW(std::runtime_error, "Error while socket() for bus [" + m_channelName+ "]: " + CanModuleerrnoToString(), this);

	struct ifreq ifr;
	memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
	if (m_channelName.length() > sizeof ifr.ifr_name-1)
	{
		MLOGSOCK(ERR,this) << "Given name of the port exceeds operating-system imposed limit";
		return -1;
	}
	strncpy(ifr.ifr_name, m_channelName.c_str(), sizeof(ifr.ifr_name)-1);

	if (ioctl(m_sock, SIOCGIFINDEX, &ifr) < 0)
		LOG_AND_THROW(std::runtime_error, "Error while ioctl(SIOCGIFINDEX) for bus [" + m_channelName+ "]: " + CanModuleerrnoToString(), this);

	can_err_mask_t err_mask = 0x1ff;
	if( setsockopt(m_sock, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof err_mask) < 0 )
		LOG_AND_THROW(std::runtime_error, "Error while setsockopt() for bus [" + m_channelName+ "]: " + CanModuleerrnoToString(), this);

	struct sockaddr_can socketAdress;
	socketAdress.can_family = AF_CAN;
	socketAdress.can_ifindex = ifr.ifr_ifindex;

	// server listening on socket
	if (::bind(m_sock, (struct sockaddr *)&socketAdress, sizeof socketAdress) < 0)
		LOG_AND_THROW(std::runtime_error, "Error while bind() for bus [" + m_channelName+ "]: " + CanModuleerrnoToString(), this);

	// Fetch initial state of the port
	int ret=can_get_state( m_channelName.c_str(), &m_errorCode );
	if ( ret ) {
		m_errorCode = ret;
		MLOGSOCK(ERR,this) << "can_get_state() failed with error code " << ret << ", it was not possible to obtain initial port state";
	}
	// updateInitialError(); // TODO piotr
	//m_statistics.setTimeSinceOpened(); // TODO Piotr uncomment it later
	return m_sock;
}


/**
 * Method that sends a message trough the can bus channel. If the method createBUS was not
 * called before this, sendMessage will fail, as there is no
 * can bus channel to send a message through.
 *
 * @param cobID Identifier that will be used for the message.
 * @param len Length of the message -- must be maximum 8 octets.
 * @param message Message to be sent trough the can bus.
 * @param rtr is the message a remote transmission request?
 * @return Was the message successfully passed to the CAN adapter?
 */
bool CSockCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{

	// we should skip this if the port is "closed" already
	if ( !m_CanScanThreadRunEnableFlag ){
		MLOGSOCK(TRC,this) << __FUNCTION__ << " bus is already closed, sending refused";
		return false;
	}

	struct can_frame canFrame = CSockCanScan::emptyCanFrame();
	if (len > 8)
		LOG_AND_THROW(
			std::logic_error,
			"Requested sending a frame longer than 8 octets [" + std::to_string(len) + "]. This is out of standard.",
			this);
	
	canFrame.can_dlc = len;
	memcpy(canFrame.data, message, len);
	canFrame.can_id = cobID;
	if (rtr)
		canFrame.can_id |= CAN_RTR_FLAG;
	
	bool success = writeWrapper(&canFrame);

	if (success)
	{
		m_statistics.onTransmit( canFrame.can_dlc );
		//m_statistics.setTimeSinceTransmitted(); TODO Piotr uncomment it later
	}
	return success;
	
}

/**
 * Method that sends a remote request trough the can bus channel. If the method createBUS was not called before this, sendMessage will fail, as there is no
 * can bus channel to send the request trough. Similar to sendMessage, but it sends an special message reserved for requests.
 * @param cobID: Identifier that will be used for the request.
 * @return: Was the initialisation process successful?
 *
 * Used in the CANopen NG server and other applications.
 */
bool CSockCanScan::sendRemoteRequest(short cobID)
{
	struct can_frame canFrame (emptyCanFrame());
	canFrame.can_id = cobID + CAN_RTR_FLAG	;
	return writeWrapper(&canFrame);
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
int CSockCanScan::createBus(const std::string& name, const std::string& parameters)
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
	m_hCanScanThread = new std::thread( &CSockCanScan::backgroundThread, this);
	MLOGSOCK(TRC,this) << "created main thread m_idCanScanThread";
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
	// m_statistics.encodeCanModuleStatus(); // TODO Piotr -- uncomment it later

	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}

// void CSockCanScan::updateInitialError ()
// {
// 	if (m_errorCode == 0) {
// 		// clearErrorMessage(); // TODO
// 	} else {
// 		timeval now (nowAsTimeval());
// 		canMessageError( m_errorCode, "Initial port state: error", now);
// 	}
// }

int CSockCanScan::selectWrapper ()
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

bool CSockCanScan::writeWrapper (const can_frame* frame)
{
	ssize_t numberOfWrittenBytes;

	numberOfWrittenBytes = write(m_sock, frame, sizeof (struct can_frame)); 
	if (numberOfWrittenBytes < 0)
	{
		if (errno == ENOBUFS)
		{
			MLOGSOCK(ERR, this) << "ENOBUFS; waiting a jiffy [100ms]...";
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			return false;
		}
		else
		{
			MLOGSOCK(ERR, this) << "write: " << CanModuleerrnoToString();
			return false;
		}
	}
	else if ((size_t)numberOfWrittenBytes < sizeof (struct can_frame)) // numberOfWrittenBytes is for sure positive
	{
		MLOGSOCK(ERR,this) << "sendRemoteRequest() sent less bytes than expected";
		return false;
	}
	else
		return true;
}

can_frame CSockCanScan::emptyCanFrame()
{
	can_frame f;
	memset((void*)&f, 0, sizeof f);
	return f;	
}

void CSockCanScan::recoverPort ()
{
	// pid_t _tid = gettid();
	auto _tid = this_thread::get_id();
	while ( m_CanScanThreadRunEnableFlag) 
	{
		MLOGSOCK(INF, this) << "Waiting 10000ms."<< " tid= " << _tid;
		std::this_thread::sleep_for(std::chrono::seconds(10));

		if ( m_sock > 0 )
		{
			// try closing the socket
			MLOGSOCK(INF, this) << "Closing socket."<< " tid= " << _tid;
			if (close(m_sock) < 0)
			{
				MLOGSOCK(ERR, this) << "Socket close error!"<< " tid= " << _tid;
			}
			m_sock = -1;
		}

		/**
		 * try to re-open the socket.
		 * only reopen the socket if we still need it.
		 * If we need to shutdown the thread, leave it closed and
		 * terminate main loop.
		 */

		MLOGSOCK(INF, this) << " tid= " << _tid << " Now port will be reopened.";
		try
		{
			m_sock = openCanPort();
			return; // was successful
		}
		catch (const std::exception& e)
		{
			MLOGSOCK(ERR, this) << " tid= " << _tid << "openCanPort() failed [" << e.what() << "], will retry in 10s";
		}
	} 
}

void CSockCanScan::publishStatus (
	unsigned int status,
	const std::string& message,
	bool unconditionalMessage)
{
	if (unconditionalMessage || (m_errorCode >= 0 && status < 0))
	{ //! Notify about transition to error.
		MLOGSOCK(ERR, this) << message << "tid=[" << this_thread::get_id() << "]";
	}
	m_errorCode = status;
	timeval now (nowAsTimeval());
	canMessageError(status, message.c_str(), now);
}

void CSockCanScan::fetchAndPublishState ()
{
	int obtainedState;
	if (can_get_state(m_channelName.c_str(), &obtainedState) < 0)
	{
		// are we in DontConfigure mode? if so, it is okay for get_state to fail
		if (m_CanParameters.m_dontReconfigure)
			return;
		else
			publishStatus(-1, "Can't get state via netlink", true);
	}
	publishStatus(obtainedState, translateCanStateToText((can_state)obtainedState));
}