/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * AnaCanScan.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: pnikiel, vfilimon, quasar team
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
 *
 * PEAK bridge integration for windows
 */

#include "pkcan.h"

#include <time.h>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/thread/thread.hpp>

#include <LogIt.h>

#include <CanModuleUtils.h>

/* static */ std::map< std::string, std::string> PKCanScan::m_busMap;

#define MLOGPK(LEVEL,THIS) LOG(Log::LEVEL, THIS->logItHandle()) << __FUNCTION__ << " " << " peak bus= " << THIS->getBusName() << " "

boost::mutex peakReconnectMutex; // protect m_busMap


bool  initLibarary =  false;
extern "C" __declspec(dllexport) CCanAccess *getCanBusAccess()
{
	CCanAccess *cc;
	cc = new PKCanScan;
	return cc;
}

PKCanScan::PKCanScan():
				m_busStatus(0),
				m_baudRate(0),
				m_idCanScanThread(0),
				m_idPeakReconnectionThread(0),
				m_CanScanThreadRunEnableFlag(false),
				m_logItHandlePk(0),
				m_pkCanHandle(0),
				m_gsig( NULL )
{
	m_statistics.setTimeSinceOpened();
	m_statistics.beginNewRun();
	m_failedSendCountdown = m_maxFailedSendCount;

}

PKCanScan::~PKCanScan()
{
	MLOGPK(DBG,this) << __FUNCTION__ <<" specific destructor calling stopBus()";
	stopBus();
}
/**
 * does not get called because advanced diag is not available for this implementation.
 * But we provide a proper return type nevertheless
 */
/* virtual */ std::vector<CanModule::PORT_LOG_ITEM_t> PKCanScan::getHwLogMessages ( unsigned int n ){
	std::vector<CanModule::PORT_LOG_ITEM_t> log;
	return log;
}
/* virtual */  CanModule::HARDWARE_DIAG_t PKCanScan::getHwDiagnostics (){
	CanModule::HARDWARE_DIAG_t d;
	return d;
}
/* virtual */ CanModule::PORT_COUNTERS_t PKCanScan::getHwCounters (){
	CanModule::PORT_COUNTERS_t c;
	return c;
}

/**
 * notify the main thread to finish and delete the bus from the map of connections
 */
void PKCanScan::stopBus ()
{
	MLOGPK(TRC, this) << __FUNCTION__;

	m_CanScanThreadRunEnableFlag = false;
	MLOGPK(TRC, this) << " try finishing thread...calling CAN_Uninitialize";
	TPCANStatus tpcanStatus = CAN_Uninitialize(m_pkCanHandle);
	MLOGPK(TRC, this) << "CAN_Uninitialize returns " << (int) tpcanStatus;
	fetchAndPublishCanPortState ();
	{
		peakReconnectMutex.lock();
		std::map< std::string, std::string>::iterator it = PKCanScan::m_busMap.find( m_busName );
		if (it != PKCanScan::m_busMap.end()) {
			m_idCanScanThread = 0;
			PKCanScan::m_busMap.erase ( it );
			MLOGPK(TRC,this) << " bus " << m_busName << " erased from map, OK";
			m_busName = "nobus";
		} else {
			MLOGPK(DBG,this) << " bus " << m_busName << " does not exist";
		}
		peakReconnectMutex.unlock();
	}
	CanModule::ms_sleep( 2000 );
	fetchAndPublishCanPortState();
	MLOGPK(DBG,this) << __FUNCTION__ << " finished";
}


/**
 * PCANBasic.h:113
 */
uint32_t PKCanScan::getPortStatus(){
	TPCANStatus stat = CAN_GetStatus( m_pkCanHandle );
	return( stat | CANMODULE_STATUS_BP_PEAK );
}


/**
 * thread to supervise port activity
 */
DWORD WINAPI PKCanScan::CanScanControlThread(LPVOID pCanScan)
{
	PKCanScan *pkCanScanPointer = reinterpret_cast<PKCanScan *>(pCanScan);
	TPCANHandle tpcanHandler = pkCanScanPointer->m_pkCanHandle;
	MLOGPK(DBG,pkCanScanPointer) << "CanScanControlThread Started. m_CanScanThreadShutdownFlag = [" << pkCanScanPointer->m_CanScanThreadRunEnableFlag <<"]";
	pkCanScanPointer->m_CanScanThreadRunEnableFlag = true;
	while ( pkCanScanPointer->m_CanScanThreadRunEnableFlag ) {
		pkCanScanPointer->fetchAndPublishCanPortState ();

		TPCANMsg tpcanMessage;
		TPCANTimestamp tpcanTimestamp;
		TPCANStatus tpcanStatus = CAN_Read(tpcanHandler,&tpcanMessage,&tpcanTimestamp);
		if ( tpcanStatus == PCAN_ERROR_OK ) {
			CanMsgStruct canMessage;
			for(int i=0; i < 8; i++) {
				canMessage.c_data[i] = tpcanMessage.DATA[i];
			}
			canMessage.c_dlc = tpcanMessage.LEN;
			canMessage.c_id = tpcanMessage.ID;
			canMessage.c_ff = tpcanMessage.MSGTYPE;

			MLOGPK(DBG, pkCanScanPointer)<< __FILE__ << " " << __LINE__ << " " << __FUNCTION__
					<< " CanModule peak received message: " << CanModule::canMessageToString( canMessage );

			unsigned long long mmsec = 0xFFFFFFFFUL;
			mmsec = mmsec *  1000;
			mmsec = mmsec * tpcanTimestamp.millis_overflow;
			mmsec = mmsec + tpcanTimestamp.micros;
			mmsec = mmsec + 1000 * tpcanTimestamp.millis;

			canMessage.c_time.tv_usec = mmsec % 1000000UL;
			canMessage.c_time.tv_sec = long(mmsec / 1000000UL);
			pkCanScanPointer->canMessageCame(canMessage);
			pkCanScanPointer->m_statistics.onReceive( canMessage.c_dlc );
			pkCanScanPointer->m_statistics.setTimeSinceReceived();

			// we can reset the reconnectionTimeout here, since we have received a message
			pkCanScanPointer->resetTimeoutOnReception();
		} else {
			if ( (tpcanStatus & PCAN_ERROR_QRCVEMPTY) && (pkCanScanPointer->getReconnectCondition() == CanModule::ReconnectAutoCondition::timeoutOnReception)
					&& pkCanScanPointer->hasTimeoutOnReception() ) {

				//send a reconnection thread trigger
				MLOGPK(DBG, pkCanScanPointer) << "trigger reconnection thread to check reception timeout " << pkCanScanPointer->getBusName();
				pkCanScanPointer->triggerReconnectionThread();
			}
		}

		// don't slow down the reception too much, but check sometimes
		static int calls = 100;
		if ( !calls-- ) {
			pkCanScanPointer->fetchAndPublishCanPortState();
			calls = 100;
		}


	}
	MLOGPK(TRC, pkCanScanPointer) << "exiting thread...(in 2 secs)";
	pkCanScanPointer->fetchAndPublishCanPortState ();
	CanModule::ms_sleep( 2000 );
	ExitThread(0);
	return 0;
}

/**
 * Method that initialises a CAN bus channel for peak@windows (using PEAK Basic)
 * All following methods called on the same object will be using this initialized channel.
 * Only USB interfaces for PEAK modules, and only NON FD modules are supported for now.
 *
 * @param name = 2 parameters separated by ":" like "n0:n1"
 * 		* n0 = "pk" for peak@windows
 * 		* n1 = CAN port number on the module, can be prefixed with "can": 0..N
 * 		* ex.: "pk:can1" speaks to port 1 (the second port) on peak module
 * 		* ex.: "pk:1" works as well
 *
 * @param parameters one parameter: "p0", positive integer
 * 				* "Unspecified" (or empty): using defaults = "125000" // params missing
 * 				* p0: bitrate: 50000, 100000, 125000, 250000, 500000, 1000000 bit/s
 *				  i.e. "250000"
 *
 * @return was the initialisation process successful:
 * 0 = ok
 * 1 = ok, bus exists already, we skip
 * -1: not ok, problem configuring the board, try again
 * -2: could not create the thread
 * -3: sth else went wrong
 */
int PKCanScan::createBus(const std::string name, const std::string parameters, float factor )
{
	m_sendThrottleDelay = (int) factor;
	return( createBus( name, parameters) );
}
int PKCanScan::createBus(const std::string name, const std::string parameters )
{
	m_busName = name;
	m_busParameters = parameters;
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
		logIt->getComponentHandle( CanModule::LogItComponentName, m_logItHandlePk );
		LOG(Log::INF, m_logItHandlePk ) << CanModule::LogItComponentName << " Dll logging initialized OK";
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
	MLOGPK(DBG, this) << " *** nb of LogIt components= " << log_comp_map.size() << std::endl;
	for ( it = log_comp_map.begin(); it != log_comp_map.end(); it++ )
	{
		Log::LOG_LEVEL level;
		Log::getComponentLogLevel( it->first, level);
		MLOGPK(DBG, this) << " *** " << " LogIt component " << it->second << " level= " << level;
	}

	MLOGPK(DBG, this) << " name= " << name << " parameters= " << parameters << ", configuring CAN board";
	if ( !m_configureCanboard(name,parameters) ) {
		MLOGPK( ERR, this ) << " name= " << name << " parameters= " << parameters << ", failed to configure CAN board";
		return (-1);
	}
	fetchAndPublishCanPortState();

	bool skipMainThreadCreation = false;
	{
		peakReconnectMutex.lock();
		// dont create a main thread for the same bus twice
		std::map<std::string, std::string>::iterator it = PKCanScan::m_busMap.find( name );
		if (it == PKCanScan::m_busMap.end()) {
			PKCanScan::m_busMap.insert ( std::pair<std::string, std::string>(name, parameters) );
			m_busName = name;
		} else {
			LOG(Log::WRN) << __FUNCTION__ << " bus exists already [" << name << ", " << parameters << "], not creating another main thread";
			skipMainThreadCreation = true;
		}
		peakReconnectMutex.unlock();
	}

	if ( skipMainThreadCreation ){
		MLOGPK(TRC, this) << "Re-using main thread m_idCanScanThread= " << m_idCanScanThread
			<< " and reconnection thread m_idPeakReconnectionThread= " << m_idPeakReconnectionThread;
		return (1);
	}	else {
		MLOGPK(TRC, this) << "creating  main thread m_idCanScanThread= " << m_idCanScanThread;
		CAN_FilterMessages(m_pkCanHandle,0,0x7FF,PCAN_MESSAGE_STANDARD);
		m_hCanScanThread = CreateThread(NULL, 0, CanScanControlThread, this, 0, &m_idCanScanThread);
		if ( NULL == m_hCanScanThread ) {
			DebugBreak();
			return (-2);
		} 

		/**
		* start a reconnection thread
		*/
		m_PeakReconnectionThread = CreateThread(NULL, 0, CanReconnectionThread, this, 0, &m_idPeakReconnectionThread);
		if (NULL == m_PeakReconnectionThread) {
			MLOGPK(TRC, this) << "could not start reconnection thread";		
			DebugBreak();
			return (-4);
		} else {
			return(0);
		}
	}
	return (-3); // sth else went wrong since we did not return before
}


/**
 * method to configure peak modules. We restrict this to USB interfaces and fixed datarate (not FD) modules
 * If needed this can relatively easily be extended to other interfaces and FD mods as well.
 *
 * These USB CAN bridges are "plug&lplay CAN devices according to peak, so we can initialize them
 * with only the handle and the baudrate. BUT - The handle is PER MODULE and not PER CHANNEL !!
 *
 * Nevertheless this is called for each BUS creation.
 *
 * returns:
 * true = success
 * false = failed
 */
bool PKCanScan::m_configureCanboard(const std::string name,const std::string parameters)
{
	m_sBusName = name;
	m_baudRate = PCAN_BAUD_125K;  // default
	m_CanParameters.m_lBaudRate = 125000;  // default

	// for FD modules
	//unsigned int parametersTseg1 = 0;
	//unsigned int parametersTseg2 = 0;
	//unsigned int parametersSjw = 0;
	//unsigned int parametersNoSamp = 0;
	//unsigned int parametersSyncmode = 0;
	//long parametersBaudRate;
	//int	numPar;

	// Process the parameters. Returns the result as a vector, and for bw compatibility puts all parameters,
	// and the bitrate, into m_CanParameters as well
	std::vector<std::string> vectorString;
	vectorString = parseNameAndParameters(name, parameters);

	for (auto & element : vectorString) {
		MLOGPK(DBG, this) << __FUNCTION__ << " element= " << element;
	}
	MLOGPK(DBG, this) << __FUNCTION__ << " m_CanParameters.m_lBaudRate= " << m_CanParameters.m_lBaudRate;

	// peak guys start counting from 1, we start counting from 0. ugh.
	int ich = atoi(vectorString[1].c_str());
	std::stringstream channel;
	channel << ich + 1;

	std::string interface = "USB";
	std::string humanReadableCode = interface + channel.str();
	m_pkCanHandle = m_getHandle( humanReadableCode.c_str() );
	MLOGPK( DBG, this ) << "PEAK handle for vectorString[1]= " << vectorString[1]
	      << " is m_pkCanHandle= 0x" <<std::hex <<  m_pkCanHandle << std::dec
		  << " human readable code= " << humanReadableCode;


	if (strcmp(parameters.c_str(), "Unspecified") != 0)
	{
		//numPar = sscanf_s(canpars, "%d %d %d %d %d %d", &parametersBaudRate, &parametersTseg1, &parametersTseg2, &parametersSjw, &parametersNoSamp, &parametersSyncmode);
		//Get the can object handler
		m_pkCanHandle = m_getHandle( humanReadableCode.c_str() /* vectorString[1].c_str() */ );
		//Process the baudRate if needed
		if (m_CanParameters.m_iNumberOfDetectedParameters == 1)
		{
			switch (m_CanParameters.m_lBaudRate)
			{
			case 50000:
				m_baudRate = PCAN_BAUD_50K;
				break;
			case 100000:
				m_baudRate = PCAN_BAUD_100K;
				break;
			case 125000:
				m_baudRate = PCAN_BAUD_125K;
				break;
			case 250000:
				m_baudRate = PCAN_BAUD_250K;
				break;
			case 500000:
				m_baudRate = PCAN_BAUD_500K;
				break;
			case 1000000:
				m_baudRate = PCAN_BAUD_1M;
				break;
			default:
				m_baudRate = m_CanParameters.m_lBaudRate;
			}
		} else {
			if  (m_CanParameters.m_iNumberOfDetectedParameters != 0) {
				m_baudRate = m_CanParameters.m_lBaudRate;
			} else {
				MLOGPK(ERR, this) << "Error while parsing parameters: this syntax is incorrect: [" << parameters << "]";
				return false;
			}
		}
	} else {
		MLOGPK(DBG, this) << "Unspecified parameters, default values will be used.";
	}
	MLOGPK(DBG, this) << " m_baudRate= 0x" << std::hex << m_baudRate << std::dec;
	MLOGPK(DBG, this) << " m_CanParameters.m_lBaudRate= " << m_CanParameters.m_lBaudRate;

	/** FD (flexible datarate) modules.
	 * we need to contruct (a complicated) bitrate string in this case, according to PEAK PCAN-Basic Documentation API manual p.82
	 * two data rates, for nominal and data, can be defined.
	 * all these parameters have to be passed
	 *TPCANStatus tpcanStatus = CAN_InitializeFD(m_canObjHandler, m_baudRate);
	 * TPCANBitrateFD br = "f_clock_mhz=20, nom_brp=5, nom_tseg1=2, nom_tseg2=1, nom_sjw=1";
	 * PCAN_BR_CLOCK_MHZ=20, PCAN_BR_NOM_BRP=5, PCAN_BR_DATA_TSEG1=2, PCAN_BR_DATA_TSEG2=1, PCAN_BR_NOM_SJW=1;
	 */
	//TPCANBitrateFD br = "f_clock_mhz=20, nom_brp=5, nom_tseg1=2, nom_tseg2=1, nom_sjw=1";
	//TPCANStatus tpcanStatus = CAN_InitializeFD(m_canObjHandler, br );


	/**
	 * fixed datarate modules (classical CAN), plug and play
	 * we try 10 times until success, the OS is a bit slow
	 */
	bool ret = true;
	MLOGPK(TRC, this) << "calling CAN_Initialize on m_pkCanHandle= " << m_pkCanHandle;
	TPCANStatus tpcanStatus = 99;
	int counter = 10;
	while ( tpcanStatus != PCAN_ERROR_OK && counter > 0 ){
		tpcanStatus = CAN_Initialize(m_pkCanHandle, m_baudRate );
		MLOGPK(TRC, this) << "CAN_Initialize returns 0x" << std::hex << (unsigned int) tpcanStatus << std::dec << " counter= " << counter;
		if ( tpcanStatus == PCAN_ERROR_OK ) {
			MLOGPK(TRC, this) << "CAN_Initialize has returned OK 0x " << std::hex << (unsigned int) tpcanStatus << std::dec;
			ret = true;
			break;
		}
		// for plug&play devices this is returned on the second channel of a USB module, apparently.
		// This seems OK, so we just continue talking to the channel. Counts as success.
		if ( tpcanStatus == PCAN_ERROR_INITIALIZE ) {
			MLOGPK(TRC, this) << "CAN_Initialize detected module is already in use (plug&play), returned OK 0x " << std::hex << (unsigned int) tpcanStatus << std::dec;
			ret = true;
			break;
		}
		CanModule::ms_sleep(1000);
		MLOGPK(TRC, this) << "try again... calling Can_Uninitialize " ;
		tpcanStatus = CAN_Uninitialize( m_pkCanHandle ); // Giving the TPCANHandle value "PCAN_NONEBUS" uninitialize all initialized channels
		MLOGPK(TRC, this) << "CAN_Uninitialize returns " << std::hex << (unsigned int) tpcanStatus << std::dec;
		CanModule::ms_sleep(1000);
		MLOGPK(TRC, this) << "try again... calling Can_Initialize " ;
		tpcanStatus = 99;
		ret = false;
		counter--;
	}
	MLOGPK(TRC, this) << "the frame sending delay is " << m_sendThrottleDelay << " us";
	return ret;
}

// send an error message down the error signal
bool PKCanScan::m_sendErrorCode(long status)
{
	if (status != m_busStatus)
	{
		timeval ftTimeStamp = CanModule::convertTimepointToTimeval(currentTimeTimeval());
		char *errorMessage;
		getErrorMessage(status, &errorMessage);
		canMessageError(status, errorMessage, ftTimeStamp );
		m_busStatus = (TPCANStatus)status;
	}
	if (status != PCAN_ERROR_OK)
	{
		status = CAN_Reset(m_pkCanHandle);
		return false;
	}
	return true;
}

/**
 * method to send a CAN message to the peak module. we use the standard API "PCAN-Basic" for this for windows
 * and we talk only over USB to fixed bitrate modules. The flexible bitrate (FD) modules can be implemented later
 * as well: for this we need more parameters to pass and a switch between CAN_Write and CAN_WriteFD.
 */
bool PKCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{


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

	MLOGPK(DBG,this) << "Sending message: [" << CanModule::canMessage2ToString(cobID, len, message, rtr) << "]";

	TPCANStatus tpcanStatus;
	TPCANMsg tpcanMessage;
	tpcanMessage.ID = cobID;
	tpcanMessage.MSGTYPE = PCAN_MESSAGE_STANDARD;
	if(rtr) {
		tpcanMessage.MSGTYPE = PCAN_MESSAGE_RTR;
	}
	int lengthOfUnsentData, lengthToBeSent;
	lengthOfUnsentData = len;
	do {
		//In every iteration of this loop a piece of message is sent of maximum 8 chars
		//To keep track of how much message is left we use vars lengthOfUnsentData and lengthToBeSent
		if (lengthOfUnsentData > 8) {
			lengthToBeSent = 8;
			lengthOfUnsentData = lengthOfUnsentData - 8;
		}
		else {
			lengthToBeSent = lengthOfUnsentData;
		}
		tpcanMessage.LEN = lengthToBeSent;

		memcpy(tpcanMessage.DATA, message, lengthToBeSent);
		tpcanStatus = CAN_Write(m_pkCanHandle, &tpcanMessage);

		MLOGPK(TRC, this) << " send message returned tpcanStatus= "	<< std::hex << "0x" << tpcanStatus << std::dec;
		if (tpcanStatus != PCAN_ERROR_OK) {
			MLOGPK(ERR, this) << " send message problem detected, tpcanStatus= " << std::hex << "0x" << tpcanStatus << std::dec;

			// here we should see if we need to reconnect
			triggerReconnectionThread();
			return m_sendErrorCode(tpcanStatus);
		} else {
			MLOGPK(TRC, this) << " send message OK, tpcanStatus= "
				<< std::hex << "0x" << tpcanStatus << std::dec;
		}
		m_statistics.onTransmit( tpcanMessage.LEN );
		m_statistics.setTimeSinceTransmitted();

		message = message + lengthToBeSent;


		// port state update, decrease load on the board a bit
		static int calls = 5;
		if ( !calls-- ) {
			fetchAndPublishCanPortState();
			calls = 5;
		}
	}
	while (lengthOfUnsentData > 8);
	return m_sendErrorCode(tpcanStatus);
}

bool PKCanScan::sendRemoteRequest(short cobID)
{
	TPCANMsg    tpcanMessage;
	tpcanMessage.ID = cobID;
	tpcanMessage.MSGTYPE = PCAN_MESSAGE_RTR;
	TPCANStatus tpcanStatus = CAN_Write(m_pkCanHandle, &tpcanMessage);
	return m_sendErrorCode(tpcanStatus);
}

bool PKCanScan::getErrorMessage(long error, char **message)
{
	char tmp[512];
	CAN_GetErrorText((TPCANStatus)error,0, tmp);
	*message = new char[strlen(tmp)+1];
	strcpy(*message,tmp);
	return true;
}

/**
 * This method returns a TPCANHandle (which is in fact a specific vendor defined hex value) as a function of a human
 * readable string. The human readable string basically codes the intefrace type (USB, ISA, PCI...) and the CAN port (1..8)
 * so that i.e. USB3->0x53 which means USB interface 3rd channel
 */
TPCANHandle PKCanScan::m_getHandle(const char *name)
{
	const char *channelNameArray[] = {
			"ISA1","ISA2","ISA3","ISA4","ISA5","ISA6","ISA7","ISA8",
			"DNG1",
			"PCI1","PCI2","PCI3","PCI4","PCI5","PCI6","PCI7","PCI8",
			"USB1","USB2","USB3","USB4","USB5","USB6","USB7","USB8",
			"PCC1","PCC2"
	};

	const TPCANHandle handlerArray[] = {
			0x21,  // PCAN-ISA interface, channel 1
			0x22,  // PCAN-ISA interface, channel 2
			0x23,  // PCAN-ISA interface, channel 3
			0x24,  // PCAN-ISA interface, channel 4
			0x25,  // PCAN-ISA interface, channel 5
			0x26,  // PCAN-ISA interface, channel 6
			0x27,  // PCAN-ISA interface, channel 7
			0x28,  // PCAN-ISA interface, channel 8

			0x31,  // PCAN-Dongle/LPT interface, channel 1

			0x41,  // PCAN-PCI interface, channel 1
			0x42,  // PCAN-PCI interface, channel 2
			0x43,  // PCAN-PCI interface, channel 3
			0x44,  // PCAN-PCI interface, channel 4
			0x45,  // PCAN-PCI interface, channel 5
			0x46,  // PCAN-PCI interface, channel 6
			0x47,  // PCAN-PCI interface, channel 7
			0x48,  // PCAN-PCI interface, channel 8

			0x51,  // PCAN-USB interface, channel 1
			0x52,  // PCAN-USB interface, channel 2
			0x53,  // PCAN-USB interface, channel 3
			0x54,  // PCAN-USB interface, channel 4
			0x55,  // PCAN-USB interface, channel 5
			0x56,  // PCAN-USB interface, channel 6
			0x57,  // PCAN-USB interface, channel 7
			0x58,  // PCAN-USB interface, channel 8

			0x61,  // PCAN-PC Card interface, channel 1
			0x62   // PCAN-PC Card interface, channel 2
	};
	int chn = sizeof(channelNameArray)/sizeof(channelNameArray[0]);
	char tmpName[] = "temp";//The previous method didn't work because of excessive string size in tmpName. Since all the names are 4 character long I just changed the size to 4
	for (unsigned int j=0; j < 4; j++) {
		tmpName[j] = toupper(name[j]);
	}
	for (int i = 0; i < chn; i++) {
		if (strcmp(channelNameArray[i],tmpName) == 0) {
			return handlerArray[i];
		}
	}
	return 0;
}

void PKCanScan::getStatistics( CanStatistics & result )
{
	m_statistics.computeDerived (m_baudRate);
	result = m_statistics;  // copy whole structure
	m_statistics.beginNewRun();
}


/**
 * Reconnection thread managing the reconnection behavior, per port. The behavior settings can not change during runtime.
 * This thread is initialized after the main thread is up, and then listens on its cond.var as a trigger.
 * Triggers occur in two contexts: sending and receiving problems.
 *
 * https://en.cppreference.com/w/cpp/thread/condition_variable/wait
 */
DWORD WINAPI PKCanScan::CanReconnectionThread(LPVOID pCanScan)
{
	PKCanScan *pkCanScanPointer = reinterpret_cast<PKCanScan *>(pCanScan);
	CanModule::ReconnectAutoCondition rcondition = pkCanScanPointer->getReconnectCondition();
	CanModule::ReconnectAction raction = pkCanScanPointer->getReconnectAction();

	std::string _tid;
	{
		std::stringstream ss;
		ss << std::this_thread::get_id();
		_tid = ss.str();
	}
	MLOGPK(TRC, pkCanScanPointer ) << "created reconnection thread tid= " << _tid;

	// need some sync to the main thread to be sure it is up and the sock is created: wait first time for init
	pkCanScanPointer->waitForReconnectionThreadTrigger();

	MLOGPK(TRC, pkCanScanPointer) << "initialized reconnection thread tid= " << _tid << ", entering loop";
	while ( true ) {

		// wait for sync: need a condition sync to step that thread once: a "trigger".
		MLOGPK(TRC, pkCanScanPointer) << "waiting reconnection thread tid= " << _tid;
		pkCanScanPointer->waitForReconnectionThreadTrigger();
		MLOGPK(TRC, pkCanScanPointer)
			<< " reconnection thread tid= " << _tid
			<< " condition "<< CCanAccess::reconnectConditionString(pkCanScanPointer->getReconnectCondition() )
			<< " action " << CCanAccess::reconnectActionString(pkCanScanPointer->getReconnectAction())
			<< " is checked, m_failedSendCountdown= "
			<< pkCanScanPointer->getFailedSendCountdown();

		pkCanScanPointer->fetchAndPublishCanPortState();

		// condition
		switch ( rcondition ){
		case CanModule::ReconnectAutoCondition::timeoutOnReception: {
			// no need to check the reception timeout, the control thread has done that already
			pkCanScanPointer->resetSendFailedCountdown(); // do the action
			break;
		}
		case CanModule::ReconnectAutoCondition::sendFail: {
			pkCanScanPointer->resetTimeoutOnReception();
			if (pkCanScanPointer->getFailedSendCountdown() > 0) {
				continue;// do nothing
			} else {
				pkCanScanPointer->resetSendFailedCountdown(); // do the action
			}
			break;
		}
		// do nothing but keep counter and timeout resetted
		case CanModule::ReconnectAutoCondition::never:
		default:{
			pkCanScanPointer->resetSendFailedCountdown();
			pkCanScanPointer->resetTimeoutOnReception();
			continue;// do nothing
			break;
		}
		} // switch

		// action
		switch ( raction ){
		case CanModule::ReconnectAction::singleBus: {
			MLOGPK(INF, pkCanScanPointer) << " reconnect condition " << CCanAccess::reconnectConditionString( rcondition )
										<< " triggered action " << CCanAccess::reconnectActionString( raction );


				CAN_Initialize(pkCanScanPointer->getTPCANHandle(), pkCanScanPointer->getBaudRate() );
				CanModule::ms_sleep( 10000 ); // USB module boot is slow, the OS is also slow to recognize the USB back
			break;
		}

		case CanModule::ReconnectAction::allBusesOnBridge: {
			MLOGPK(INF, pkCanScanPointer) << " reconnect condition " << CCanAccess::reconnectConditionString( rcondition )
				<< " triggered action " << CCanAccess::reconnectActionString( raction );
			break;
		}
		default: {
			// we have a runtime bug
			MLOGPK(ERR, pkCanScanPointer) << "reconnection action "
					<< (int) raction << CCanAccess::reconnectActionString( raction )
					<< " unknown. Check your config & see documentation. No action.";
			break;
		}
		} // switch
	} // while
	MLOGPK(TRC, pkCanScanPointer) << "exiting thread...(in 2 secs)";
	pkCanScanPointer->fetchAndPublishCanPortState();
	CanModule::ms_sleep( 2000 );
	ExitThread(0);
	return 0;
}


std::string PKCanScan::m_translatePeakPortStatusBitpatternToText( TPCANStatus bp ){
	std::string msg = " peak port status message: ";
	MLOGPK(TRC, this) << " TPCANStatus= 0x" << std::hex << bp << std::dec;

	if ( bp == PCAN_ERROR_OK ) msg += "PCAN_ERROR_OK (000000) No error. Success. "; 
	if ( bp & PCAN_ERROR_XMTFULL ) msg += "PCAN_ERROR_XMTFULL (000001) Transmit buffer in CAN controller is full. ";
	if ( bp & PCAN_ERROR_OVERRUN ) msg += "PCAN_ERROR_OVERRUN (000002) CAN controller was read too late. ";
	if ( bp & PCAN_ERROR_BUSLIGHT ) msg += "PCAN_ERROR_BUSLIGHT (000004) Bus error: an error counter reached the 'light' limit. ";
	if ( bp & PCAN_ERROR_BUSHEAVY ) msg += "PCAN_ERROR_BUSHEAVY  (000008) Bus error: an error counter reached the 'heavy' limit. ";
	if ( bp & PCAN_ERROR_BUSWARNING ) msg += "PCAN_ERROR_BUSWARNING (000008) Bus error: an error counter reached the 'warning' limit. ";
	if ( bp & PCAN_ERROR_BUSPASSIVE ) msg += "PCAN_ERROR_BUSPASSIVE (262144) Bus error: the CAN controller is error passive. ";
	if ( bp & PCAN_ERROR_BUSOFF ) msg += "PCAN_ERROR_BUSOFF (000016)  Bus error: the CAN controller is in bus-off state. ";
	if ( bp & PCAN_ERROR_ANYBUSERR ) msg += "PCAN_ERROR_ANYBUSERR (262172) Mask for all bus errors. "; // used ?

	if ( bp & PCAN_ERROR_QRCVEMPTY ) msg += "PCAN_ERROR_QRCVEMPTY (000032) Receive queue is empty. ";
	if ( bp & PCAN_ERROR_QOVERRUN ) msg += "PCAN_ERROR_QOVERRUN (000064) Receive queue was read too late. ";
	if ( bp & PCAN_ERROR_QXMTFULL ) msg += "PCAN_ERROR_QXMTFULL (000128) Transmit queue is full. ";
	if ( bp & PCAN_ERROR_REGTEST ) msg += "PCAN_ERROR_REGTEST (000256) Test of the CAN controller hardware registers failed (no hardware found). ";
	if ( bp & PCAN_ERROR_NODRIVER ) msg += "PCAN_ERROR_NODRIVER (000512) Driver not loaded. ";
	if ( bp & PCAN_ERROR_HWINUSE ) msg += "PCAN_ERROR_HWINUSE (001024) PCAN-Hardware already in use by a PCAN-Net. ";
	if ( bp & PCAN_ERROR_NETINUSE ) msg += "PCAN_ERROR_NETINUSE (002048) A PCAN-Client is already connected to the PCAN-Net. ";
	if ( bp & PCAN_ERROR_ILLHW ) msg += "PCAN_ERROR_ILLHW (005120) PCAN-Hardware handle is invalid. ";
	if ( bp & PCAN_ERROR_ILLNET ) msg += "PCAN_ERROR_ILLNET(006144) PCAN-Net handle is invalid. ";

	if ( bp & PCAN_ERROR_ILLCLIENT ) msg += "PCAN_ERROR_ILLCLIENT (007168) PCAN-Client handle is invalid. ";
	if ( bp & PCAN_ERROR_ILLHANDLE ) msg += "PCAN_ERROR_ILLHANDLE (007168) Mask for all handle errors. "; // used ?
	if ( bp & PCAN_ERROR_RESOURCE ) msg += "PCAN_ERROR_RESOURCE (008192) Resource (FIFO, Client, timeout) cannot be created. ";
	if ( bp & PCAN_ERROR_ILLPARAMTYPE ) msg += "PCAN_ERROR_ILLPARAMTYPE (016384) Invalid parameter. ";
	if ( bp & PCAN_ERROR_UNKNOWN ) msg += "PCAN_ERROR_UNKNOWN (065536) Unknown error ";
	if ( bp & PCAN_ERROR_ILLDATA ) msg += "PCAN_ERROR_ILLDATA (131072) Invalid data, function, or action. ";
#if 0
	// not in API 17.nov.2017
	if ( bp & PCAN_ERROR_ILLMODE ) msg += "PCAN_ERROR_ILLMODE (524288) Driver object state is wrong for the attempted operation. ";
#endif
	if ( bp & PCAN_ERROR_CAUTION ) msg += "PCAN_ERROR_CAUTION (33554432) Operation succeeded but with irregularities. ";
	if ( bp & PCAN_ERROR_INITIALIZE ) msg += "PCAN_ERROR_INITIALIZE (67108864) Channel is not initialized. ";
	if ( bp & PCAN_ERROR_ILLOPERATION ) msg += "PCAN_ERROR_ILLOPERATION (134217728) Invalid operation ";

	return ( msg );
}

/**
 * (virtual) forced implementation. Generally: do whatever shenanigans you need on the vendor API and fill in the portState accordingly, stay
 * close to the semantics of the enum.
 *
 * windows peak specific implementation: we get USB and CAN status from the API PCAN-Basic API 26.july.2022 https://www.peak-system.com.
 * This is very complete ;-). So we use the standardized CanModule::CanModule_bus_state:: and ADD the specific text message from peak so that we do not
 * loose anything.
 *
 * Note that the values of the different PCAN-Status definitions are able to be bitwise combined. In
 * some cases it is possible to get more than one error code as result of calling a function.
 * Note that the values of PCAN_ERROR_INITIALIZE and PCAN_ERROR_ILLOPERATION were
 * changed!
 * PCAN_ERROR_INITIALIZE changed from 0x40000 to 0x4000000
 * PCAN_ERROR_ILLOPERATION changed from 0x80000 to 0x8000000
 *
 * PCAN_ERROR_OK 0x00000 No error. Success. (000000)
 * PCAN_ERROR_XMTFULL 0x00001 (000001) Transmit buffer in CAN controller is full.
 * PCAN_ERROR_OVERRUN 0x00002 (000002) CAN controller was read too late.
 * PCAN_ERROR_BUSLIGHT 0x00004 (000004) Bus error: an error counter reached the 'light' limit.
 * PCAN_ERROR_BUSHEAVY 0x00008 (000008) Bus error: an error counter reached the 'heavy' limit.
 * PCAN_ERROR_BUSWARNING 0x00008 (000008) Bus error: an error counter reached the 'warning' limit.
 * PCAN_ERROR_BUSPASSIVE 0x40000 (262144) Bus error: the CAN controller is error passive.
 * PCAN_ERROR_BUSOFF 0x00010 (000016)  Bus error: the CAN controller is in bus-off state.
 * PCAN_ERROR_ANYBUSERR 0x4001C (262172) Mask for all bus errors.
 *
 * ok:
 * PCAN_ERROR_QRCVEMPTY 0x00020 (000032) Receive queue is empty.
 *
 * warning:
 * PCAN_ERROR_QOVERRUN 0x00040 (000064) Receive queue was read too late.
 * PCAN_ERROR_QXMTFULL 0x00080 (000128) Transmit queue is full.
 *
 * error:
 * PCAN_ERROR_REGTEST 0x00100 (000256) Test of the CAN controller hardware registers failed (no hardware found).
 * PCAN_ERROR_NODRIVER 0x00200 (000512) Driver not loaded.
 * PCAN_ERROR_HWINUSE 0x00400 (001024) PCAN-Hardware already in use by a PCAN-Net.
 * PCAN_ERROR_NETINUSE 0x00800 (002048) A PCAN-Client is already connected to the PCAN-Net.
 * PCAN_ERROR_ILLHW 0x01400 (005120) PCAN-Hardware handle is invalid.
 * PCAN_ERROR_ILLNET 0x01800 (006144) PCAN-Net handle is invalid.
 * PCAN_ERROR_ILLCLIENT 0x01C00 (007168) PCAN-Client handle is invalid.
 * PCAN_ERROR_ILLHANDLE 0x01C00 (007168) Mask for all handle errors.
 *
 * error:
 * PCAN_ERROR_RESOURCE 0x02000 (008192) Resource (FIFO, Client, timeout) cannot be created.
 * PCAN_ERROR_ILLPARAMTYPE 0x04000  (016384) Invalid parameter.
 * PCAN_ERROR_ILLPARAMVAL 0x08000 (032768) Invalid parameter value.
 * PCAN_ERROR_UNKNOWN 0x10000 (065536) Unknown error
 * PCAN_ERROR_ILLDATA 0x20000 (131072) Invalid data, function, or action.
 * PCAN_ERROR_ILLMODE 0x80000 (524288) Driver object state is wrong for the attempted operation.
 *
 * warning:
 * PCAN_ERROR_CAUTION 0x2000000 (33554432) Operation succeeded but with irregularities.
 *
 * error:
 * PCAN_ERROR_INITIALIZE* 0x4000000 (67108864) Channel is not initialized.
 * PCAN_ERROR_ILLOPERATION* 0x8000000 (134217728) Invalid operation
 */
/* virtual */ void PKCanScan::fetchAndPublishCanPortState ()
{
	int portState = CanModule::CanModule_bus_state::CANMODULE_NOSTATE;
	TPCANStatus peak_state = CAN_GetStatus( m_pkCanHandle );

	if (peak_state == PCAN_ERROR_OK) {
		portState = CanModule::CanModule_bus_state::CANMODULE_OK; 
		publishPortStatusChanged(portState);
		return; // no further decoding needed
	}
	else if ( peak_state & PCAN_ERROR_BUSLIGHT ) {
		portState = CanModule::CanModule_bus_state::CAN_STATE_ERROR_ACTIVE;
	}
	else if (peak_state & PCAN_ERROR_BUSHEAVY) {
		portState = CanModule::CanModule_bus_state::CAN_STATE_MAX;
	}
	else if (peak_state & PCAN_ERROR_BUSPASSIVE) {
		portState = CanModule::CanModule_bus_state::CAN_STATE_ERROR_PASSIVE;
	}
	else if (peak_state & PCAN_ERROR_BUSOFF) {
		portState = CanModule::CanModule_bus_state::CAN_STATE_BUS_OFF;
	}

	/** if we reach here, means we might be OK. Lets translate all other errors into (excluding)
	  CanModule extension, to cover non-socketcan implementations
		CANMODULE_NOSTATE,   // could not get state
		CANMODULE_WARNING,   // degradation but might recover
		CANMODULE_ERROR,     // error likely stemming from SW/HW/firmware
		CANMODULE_TIMEOUT_OK, // bus is fine, no traffic
		CANMODULE_OK         // bus is fine
	 */

	// info, still ok
	if ( peak_state & PCAN_ERROR_QRCVEMPTY ){
		portState = CanModule::CanModule_bus_state::CANMODULE_TIMEOUT_OK;
		std::string msg = CanModule::translateCanBusStateToText((CanModule::CanModule_bus_state) portState ) + m_translatePeakPortStatusBitpatternToText( peak_state );
		MLOGPK(INF, this) << msg << "tid=[" << std::this_thread::get_id() << "]";
		publishPortStatusChanged( portState );
		return;
	}

	// warnings
	if ( (peak_state & PCAN_ERROR_QOVERRUN) || (peak_state & PCAN_ERROR_QXMTFULL) || (peak_state & PCAN_ERROR_CAUTION) ){
		portState = CanModule::CanModule_bus_state::CANMODULE_WARNING;
		std::string msg = CanModule::translateCanBusStateToText((CanModule::CanModule_bus_state) portState ) + m_translatePeakPortStatusBitpatternToText( peak_state );
		MLOGPK(WRN, this) << msg << "tid=[" << std::this_thread::get_id() << "]";
		publishPortStatusChanged( portState );
		return;
	}

	// all the rest are errors
	if ( peak_state != PCAN_ERROR_OK ){
		portState = CanModule::CanModule_bus_state::CANMODULE_ERROR;
		std::string msg = CanModule::translateCanBusStateToText((CanModule::CanModule_bus_state) portState ) + m_translatePeakPortStatusBitpatternToText( peak_state );
		MLOGPK(ERR, this) << msg << "tid=[" << std::this_thread::get_id() << "]";
		publishPortStatusChanged( portState );
		return;
	}

	// normally: never reached
	portState = CanModule::CanModule_bus_state::CANMODULE_NOSTATE;
	std::string msg = CanModule::translateCanBusStateToText((CanModule::CanModule_bus_state) portState ) + m_translatePeakPortStatusBitpatternToText( peak_state );
	MLOGPK(TRC, this) << msg << "tid=[" << std::this_thread::get_id() << "]";
	publishPortStatusChanged( portState );
}

