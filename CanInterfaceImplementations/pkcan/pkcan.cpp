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
#include <string.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/thread/thread.hpp>

#include <LogIt.h>

#include "CanModuleUtils.h"

/* static */ std::map<string, string> PKCanScan::m_busMap;

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
				m_pkCanHandle(0)
{
	m_statistics.setTimeSinceOpened();
	m_statistics.beginNewRun();
	m_failedSendCountdown = m_maxFailedSendCount;

	/**
	 * start a reconnection thread
	 */
	m_PeakReconnectionThread = CreateThread(NULL, 0, CanReconnectionThread, this, 0, &m_idPeakReconnectionThread);
	if ( NULL == m_PeakReconnectionThread ) {
		MLOGPK(TRC, this) << "could not start reconnection thread";
	}
}

PKCanScan::~PKCanScan()
{
	MLOGPK(DBG,this) << __FUNCTION__ <<" specific destructor calling stopBus()";
	stopBus();
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

	{
		peakReconnectMutex.lock();
		std::map<string, string>::iterator it = PKCanScan::m_busMap.find( m_busName );
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


#if 0
				// timeout
				/**
				 * lets check the timeoutOnReception reconnect condition. If it is true, all we can do is to
				 * close/open the port again since the underlying hardware is hidden by socketcan abstraction.
				 * Like his we do not have to pollute the "sendMessage" like for anagate, and that is cleaner.
				 */
				CanModule::ReconnectAutoCondition rcond = pkCanScanPointer->getReconnectCondition();
				CanModule::ReconnectAction ract = pkCanScanPointer->getReconnectAction();
				if ( rcond == CanModule::ReconnectAutoCondition::timeoutOnReception && pkCanScanPointer->hasTimeoutOnReception()) {
					if ( ract == CanModule::ReconnectAction::singleBus ){
						MLOGPK(INF, pkCanScanPointer) << " reconnect condition " << (int) rcond
								<< pkCanScanPointer->reconnectConditionString(rcond)
								<< " triggered action " << (int) ract
								<< pkCanScanPointer->reconnectActionString(ract);
						pkCanScanPointer->resetTimeoutOnReception();  // renew timeout while reconnect is in progress

						// deinit single bus and reopen. We do not have a openCanPort() method
						CAN_Initialize( tpcanHandler, pkCanScanPointer->m_baudRate );
						MLOGPK(TRC, pkCanScanPointer) << "reconnect one CAN port  m_UcanHandle= " << pkCanScanPointer->m_pkCanHandle;
					} else {
						MLOGPK(INF, pkCanScanPointer) << "reconnect action " << (int) ract
								<< pkCanScanPointer->reconnectActionString(ract)
								<< " is not implemented for peak";
					}
				}  // reconnect condition

				continue;
#endif
			}

#if 0

			// default behaviour: reopen the port
			pkCanScanPointer->sendErrorCode(tpcanStatus);
			if (tpcanStatus | PCAN_ERROR_ANYBUSERR) {
				CAN_Initialize(tpcanHandler,pkCanScanPointer->m_baudRate);
				Sleep(100);
			}
#endif
		}
	}
	MLOGPK(TRC, pkCanScanPointer) << "exiting thread...(in 2 secs)";
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
int PKCanScan::createBus(const string name ,const string parameters )
{
	m_busName = name;
	m_busParameters = parameters;

	LogItInstance* logItInstance = CCanAccess::getLogItInstance(); // actually calling instance method, not class
	if (!LogItInstance::setInstance(logItInstance))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not set LogIt instance" << std::endl;

	if (!logItInstance->getComponentHandle( CanModule::LogItComponentName, m_logItHandlePk))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not get LogIt component handle for " << LogItComponentName << std::endl;

	MLOGPK(DBG, this) << " name= " << name << " parameters= " << parameters << ", configuring CAN board";

	if ( !configureCanboard(name,parameters) ) {
		MLOGPK( ERR, this ) << " name= " << name << " parameters= " << parameters << ", failed to configure CAN board";
		return (-1);
	}

	bool skipMainThreadCreation = false;
	{
		peakReconnectMutex.lock();
		// dont create a main thread for the same bus twice
		std::map<string, string>::iterator it = PKCanScan::m_busMap.find( name );
		if (it == PKCanScan::m_busMap.end()) {
			PKCanScan::m_busMap.insert ( std::pair<string, string>(name, parameters) );
			m_busName = name;
		} else {
			LOG(Log::WRN) << __FUNCTION__ << " bus exists already [" << name << ", " << parameters << "], not creating another main thread";
			skipMainThreadCreation = true;
		}
		peakReconnectMutex.unlock();
	}
	if ( skipMainThreadCreation ){
		MLOGPK(TRC, this) << "Re-using main thread m_idCanScanThread= " << m_idCanScanThread;
		return (1);
	}	else {
		MLOGPK(TRC, this) << "creating  main thread m_idCanScanThread= " << m_idCanScanThread;
		CAN_FilterMessages(m_pkCanHandle,0,0x7FF,PCAN_MESSAGE_STANDARD);
		m_hCanScanThread = CreateThread(NULL, 0, CanScanControlThread, this, 0, &m_idCanScanThread);
		if ( NULL == m_hCanScanThread ) {
			DebugBreak();
			return (-2);
		} else {
			return(0);
		}
	}
	return (-3); // sth else went wrong since we did not return before
}


/**
 * method to configure peak modules, one channel at a time. We restrict this to USB interfaces and fixed datarate (not FD) modules
 * If needed this can relatively easily be extended to other interfaces and FD mods as well.
 */
bool PKCanScan::configureCanboard(const string name,const string parameters)
{
	m_sBusName = name;
	m_baudRate = PCAN_BAUD_125K;
	m_CanParameters.m_lBaudRate = 125000;

	// for FD modules
	//unsigned int parametersTseg1 = 0;
	//unsigned int parametersTseg2 = 0;
	//unsigned int parametersSjw = 0;
	//unsigned int parametersNoSamp = 0;
	//unsigned int parametersSyncmode = 0;
	//long parametersBaudRate;
	//int	numPar;
	//Process the parameters
	vector<string> vectorString;
	vectorString = parseNameAndParameters(name, parameters);
	MLOGPK(DBG, this) << " calling getHandle vectorString[1]= " << vectorString[1];

	// peak guys start counting from 1, we start counting from 0. ugh.
	int ich = atoi(vectorString[1].c_str());
	stringstream channel;
	channel << ich + 1;

	string interface = "USB";
	string humanReadableCode = interface + channel.str();
	m_pkCanHandle = getHandle( humanReadableCode.c_str() );
	MLOGPK( DBG, this ) << "PEAK handle for vectorString[1]= " << vectorString[1]
	      << " is code= 0x" <<hex <<  m_pkCanHandle << dec
		  << " human readable code= " << humanReadableCode;


	if (strcmp(parameters.c_str(), "Unspecified") != 0)
	{
		//numPar = sscanf_s(canpars, "%d %d %d %d %d %d", &parametersBaudRate, &parametersTseg1, &parametersTseg2, &parametersSjw, &parametersNoSamp, &parametersSyncmode);
		//Get the can object handler
		m_pkCanHandle = getHandle( humanReadableCode.c_str() /* vectorString[1].c_str() */ );
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
	MLOGPK(DBG, this) << " m_baudRate= 0x" << hex << m_baudRate << dec;
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
	MLOGPK(TRC, this) << "calling CAN_Initialize";
	TPCANStatus tpcanStatus = 99;
	int counter = 10;
	while ( tpcanStatus != 0 && counter > 0 ){
		tpcanStatus = CAN_Initialize(m_pkCanHandle, m_baudRate );
		MLOGPK(TRC, this) << "CAN_Initialize returns " << (int) tpcanStatus << " counter= " << counter;
		if ( tpcanStatus == 0 ) {
			MLOGPK(TRC, this) << "CAN_Initialize returns " << (int) tpcanStatus << " OK";
			break;
		}
		Sleep( 1000 ); // 1000 ms
		MLOGPK(TRC, this) << "try again... calling Can_Uninitialize " ;
		tpcanStatus = CAN_Uninitialize( m_pkCanHandle );
		Sleep( 1000 ); // 1000 ms
		counter--;
	}

	/** fixed data rate, non plug-and-play
	 * static TPCANStatus Initialize(
	 * TPCANHandle Channel,
	 * TPCANBaudrate Btr0Btr1,
	 * TPCANType HwType,
	 * UInt32 IOPort,
	 * UInt16 Interrupt);
	 */
	// TPCANStatus tpcanStatus = CAN_Initialize(m_canObjHandler, m_baudRate,256,3); // one param missing ? 
	return tpcanStatus == PCAN_ERROR_OK;
}


bool PKCanScan::sendErrorCode(long status)
{
	if (status != m_busStatus)
	{
		timeval ftTimeStamp = convertTimepointToTimeval(currentTimeTimeval());
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
	MLOGPK(DBG,this) << "Sending message: [" << ( message == 0  ? "" : (const char *) message) << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";

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
		//In every iteration of this loop a piece of message is sent of maximun 8 chars
		//To keep track of how much message is left we use vars lengthOfUnsentData and lengthToBeSent
		if (lengthOfUnsentData > 8) {
			lengthToBeSent = 8;
			lengthOfUnsentData = lengthOfUnsentData - 8;
		} else {
			lengthToBeSent = lengthOfUnsentData;
		}
		tpcanMessage.LEN = lengthToBeSent;

		memcpy(tpcanMessage.DATA,message,lengthToBeSent);
		tpcanStatus = CAN_Write(m_pkCanHandle, &tpcanMessage);
		if (tpcanStatus != PCAN_ERROR_OK) {

			// here we should see if we need to reconnect
			switch( m_reconnectCondition ){
			case CanModule::ReconnectAutoCondition::sendFail: {
				MLOGPK(WRN, this) << " detected a sendFail, triggerCounter= " << m_failedSendCountdown
						<< " failedSendCounter= " << m_maxFailedSendCount;
				m_failedSendCountdown--;
				break;
			}
			case CanModule::ReconnectAutoCondition::never:
			default:{
				m_failedSendCountdown = m_maxFailedSendCount;
				break;
			}
			}

			switch ( m_reconnectAction ){
			case CanModule::ReconnectAction::allBusesOnBridge: {
				if ( m_failedSendCountdown <= 0 ){
					MLOGPK(INF, this) << " reconnect condition " << (int) m_reconnectCondition
							<< reconnectConditionString(m_reconnectCondition)
							<< " triggered action " << (int) m_reconnectAction
							<< reconnectActionString(m_reconnectAction)
							<< " not yet implemented";
					m_failedSendCountdown = m_maxFailedSendCount;
				}
				break;
			}
			case CanModule::ReconnectAction::singleBus: {
				MLOGPK(INF, this) << " reconnect condition " << (int) m_reconnectCondition
						<< reconnectConditionString(m_reconnectCondition)
						<< " triggered action " << (int) m_reconnectAction
						<< reconnectActionString(m_reconnectAction);
				if ( m_failedSendCountdown <= 0 ){
					stopBus();
					createBus(m_busName, m_busParameters );
					MLOGPK(TRC, this) << "reconnect one CAN port  m_pkCanHandle= " << m_pkCanHandle;
					m_failedSendCountdown = m_maxFailedSendCount;
				}
				break;
			}
			default: {
				break;
			}
			}


			return sendErrorCode(tpcanStatus);
		}
		m_statistics.onTransmit( tpcanMessage.LEN );
		m_statistics.setTimeSinceTransmitted();

		message = message + lengthToBeSent;
	}
	while (lengthOfUnsentData > 8);
	return sendErrorCode(tpcanStatus);
}

bool PKCanScan::sendRemoteRequest(short cobID)
{
	TPCANMsg    tpcanMessage;
	tpcanMessage.ID = cobID;
	tpcanMessage.MSGTYPE = PCAN_MESSAGE_RTR;
	TPCANStatus tpcanStatus = CAN_Write(m_pkCanHandle, &tpcanMessage);
	return sendErrorCode(tpcanStatus);
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
TPCANHandle PKCanScan::getHandle(const char *name)
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

	std::string _tid;
	{
		std::stringstream ss;
		ss << this_thread::get_id();
		_tid = ss.str();
	}
	MLOGPK(TRC, pkCanScanPointer ) << "created reconnection thread tid= " << _tid;

	// need some sync to the main thread to be sure it is up and the sock is created: wait first time for init
	waitForReconnectionThreadTrigger();

	/**
	 * lets check the timeoutOnReception reconnect condition. If it is true, all we can do is to
	 * close/open the socket again since the underlying hardware is hidden by socketcan abstraction.
	 * Like his we do not have to pollute the "sendMessage" like for anagate, and that is cleaner.
	 */
	MLOGPK(TRC, pkCanScanPointer) << "initialized reconnection thread tid= " << _tid << ", entering loop";
	while ( true ) {

		// wait for sync: need a condition sync to step that thread once: a "trigger".
		MLOGPK(TRC, pkCanScanPointer) << "waiting reconnection thread tid= " << _tid;
		waitForReconnectionThreadTrigger();
		MLOGPK(TRC, pkCanScanPointer)
			<< " reconnection thread tid= " << _tid
			<< " condition "<< reconnectConditionString(getReconnectCondition() )
			<< " action " << reconnectActionString(getReconnectAction())
			<< " is checked, m_failedSendCountdown= "
			<< m_failedSendCountdown;

		// condition
		switch ( getReconnectCondition() ){
		case CanModule::ReconnectAutoCondition::timeoutOnReception: {
			// no need to check the reception timeout, the control thread has done that already
			resetSendFailedCountdown(); // do the action
			break;
		}
		case CanModule::ReconnectAutoCondition::sendFail: {
			resetTimeoutOnReception();
			if (m_failedSendCountdown > 0) {
				continue;// do nothing
			} else {
				resetSendFailedCountdown(); // do the action
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
		switch ( pkCanScanPointer->getReconnectAction() ){
		case CanModule::ReconnectAction::singleBus: {
			MLOGPK(INF, pkCanScanPointer) << " reconnect condition " << reconnectConditionString(m_reconnectCondition)
										<< " triggered action " << reconnectActionString(m_reconnectAction);

			pkCanScanPointer->sendErrorCode(tpcanStatus);
			if (tpcanStatus | PCAN_ERROR_ANYBUSERR) {
				CAN_Initialize(tpcanHandler,pkCanScanPointer->m_baudRate);
				CanModule::ms_sleep( 10000 );
			}
			break;
		}

		case CanModule::ReconnectAction::allBusesOnBridge: {
			MLOGPK(INF, pkCanScanPointer) << " reconnect condition " << reconnectConditionString(m_reconnectCondition)
										<< " triggered action " << reconnectActionString(m_reconnectAction);
			break;
		}
		default: {
			// we have a runtime bug
			MLOGPK(ERR, pkCanScanPointer) << "reconnection action "
					<< (int) m_reconnectAction << reconnectActionString( m_reconnectAction )
					<< " unknown. Check your config & see documentation. No action.";
			break;
		}
		} // switch
	} // while
	MLOGPK(TRC, pkCanScanPointer) << "exiting thread...(in 2 secs)";
	CanModule::ms_sleep( 2000 );
	ExitThread(0);
	return 0;
}

