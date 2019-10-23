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
#include "CanModuleUtils.h"

#include <LogIt.h>

/* static */ bool PKCanScan::s_logItRegisteredPk = false;
/* static */ Log::LogComponentHandle PKCanScan::s_logItHandlePk = 0;
/* static */ std::map<string, string> PKCanScan::m_busMap;

#define MLOGPK(LEVEL,THIS) LOG(Log::LEVEL, PKCanScan::s_logItHandlePk) << __FUNCTION__ << " " << " peak bus= " << THIS->getBusName() << " "

// using namespace std;

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
				m_CanScanThreadRunEnableFlag(false)
{
	m_statistics.beginNewRun();
}

PKCanScan::~PKCanScan()
{
	stopBus();

	// m_CanScanThreadRunEnableFlag = false;
	////	CloseHandle(m_ReadEvent);

	//CAN_Uninitialize(m_canObjHandler);
}

/**
 * notify the main thread to finish and delete the bus from the map of connections
 */
void PKCanScan::stopBus ()
{
	MLOGPK(DBG, PKCanScan::s_logItHandlePk ) << __FUNCTION__ << " m_busName= " <<  m_busName ;
	CAN_Uninitialize(m_canObjHandler);

	// notify the thread that it should finish.
	m_CanScanThreadRunEnableFlag = false;
	MLOGPK(DBG,s_logItHandlePk) << " try joining threads...";

//#if 0
	std::map<string, string>::iterator it = PKCanScan::m_busMap.find( m_busName );
	if (it != PKCanScan::m_busMap.end()) {

		// windows does not have pthread_join
		//pthread_join( m_hCanScanThread, 0 );
		m_idCanScanThread = 0;
		PKCanScan::m_busMap.erase ( it );
		m_busName = "nobus";
	} else {
		MLOGPK(DBG,s_logItHandlePk) << " not joining threads... bus does not exist";
	}
//#endif
	MLOGPK(DBG,s_logItHandlePk) << "stopBus() finished";
}


/**
 * thread to supervise port activity
 */
DWORD WINAPI PKCanScan::CanScanControlThread(LPVOID pCanScan)
{
	PKCanScan *pkCanScanPointer = reinterpret_cast<PKCanScan *>(pCanScan);
	TPCANHandle tpcanHandler = pkCanScanPointer->m_canObjHandler;
	MLOGPK(DBG,pkCanScanPointer) << "CanScanControlThread Started. m_CanScanThreadShutdownFlag = [" << pkCanScanPointer->m_CanScanThreadRunEnableFlag <<"]";
	pkCanScanPointer->m_CanScanThreadRunEnableFlag = true;
	while ( pkCanScanPointer->m_CanScanThreadRunEnableFlag ) {
		TPCANMsg tpcanMessage;
		TPCANTimestamp tpcanTimestamp;
		TPCANStatus tpcanStatus = CAN_Read(tpcanHandler,&tpcanMessage,&tpcanTimestamp);
		if (tpcanStatus == PCAN_ERROR_OK) {
			CanMsgStruct canMessage;
			for(int i=0; i < 8; i++)
			{
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
		} else {
			if (tpcanStatus & PCAN_ERROR_QRCVEMPTY) {
				continue;
			}
			pkCanScanPointer->sendErrorCode(tpcanStatus);
			if (tpcanStatus | PCAN_ERROR_ANYBUSERR) {
				CAN_Initialize(tpcanHandler,pkCanScanPointer->m_baudRate);
				Sleep(100);
			}
		}
	}
	MLOGPK(TRC, pkCanScanPointer) << "exiting thread...";
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
 * 		* ex.: "pk:can1" speaks to port 1 (the second port) on peak module at the ip
 * 		* ex.: "pk:1" works as well
 *
 * @param parameters one parameter: "p0", positive integer
 * 				* "Unspecified" (or empty): using defaults = "125000" // params missing
 * 				* p0: bitrate: 50000, 100000, 125000, 250000, 500000, 1000000 bit/s
 *				  i.e. "250000"
 *
 * @return was the initialisation process successful?
 */
bool PKCanScan::createBus(const string name ,const string parameters )
{
	m_busName = name;
	m_busParameters = parameters;

	// calling base class to get the instance from there
	Log::LogComponentHandle myHandle;
	LogItInstance* logItInstance = CCanAccess::getLogItInstance(); // actually calling instance method, not class

	// register peak@W component for logging
	if (!LogItInstance::setInstance(logItInstance))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not set LogIt instance" << std::endl;

	if (!logItInstance->getComponentHandle( CanModule::LogItComponentName, myHandle))
		std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__
		<< " could not get LogIt component handle for " << LogItComponentName << std::endl;

	PKCanScan::s_logItHandlePk = myHandle;
	MLOGPK(DBG, this) << " name= " << name << " parameters= " << parameters << ", configuring CAN board";

	m_sBusName = name; // maybe this can be cleaned up: we have m_busName already
	//We configure the canboard
	if ( !configureCanboard(name,parameters) ) {
		//If we can't initialise the canboard, the thread is not started at all
		MLOGPK( ERR, this ) << " name= " << name << " parameters= " << parameters << ", failed to configure CAN board";
		return false;
	}

	// dont create a main thread for the same bus twice
	bool skipMainThreadCreation = false;
	std::map<string, string>::iterator it = PKCanScan::m_busMap.find( name );
	if (it == PKCanScan::m_busMap.end()) {
		PKCanScan::m_busMap.insert ( std::pair<string, string>(name, parameters) );
		m_busName = name;
	} else {
		LOG(Log::WRN) << __FUNCTION__ << " bus exists already [" << name << ", " << parameters << "], not creating another main thread";
		skipMainThreadCreation = true;
	}

	if ( skipMainThreadCreation ){
		MLOGPK(TRC, this) << "Re-using main thread m_idCanScanThread= " << m_idCanScanThread;
	}	else {
		// Otherwise, we initialise the Scan thread
		CAN_FilterMessages(m_canObjHandler,0,0x7FF,PCAN_MESSAGE_STANDARD);
		m_hCanScanThread = CreateThread(NULL, 0, CanScanControlThread, this, 0, &m_idCanScanThread);
		if ( NULL == m_hCanScanThread ) {
			DebugBreak();
			return false;
		}
	}

	return true;
}


/**
 * method to configure peak modules, one channel at a time. We restrict this to USB interfaces and fixed datarate (not FD) modules
 * If needed this can relatively easily be extended to other interfaces and FD mods as well.
 */
bool PKCanScan::configureCanboard(const string name,const string parameters)
{
	m_sBusName = name;
	m_baudRate = PCAN_BAUD_125K;

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
	MLOGPK(DBG, this) << " calling getHandle vectorString[1]= " << vectorString[1] << std::endl;

	// peak guys start counting from 1, we start counting from 0. ugh.
	int ich = atoi(vectorString[1].c_str());
	stringstream channel;
	channel << ich + 1;

	string interface = "USB";
	string humanReadableCode = interface + channel.str();
	m_canObjHandler = getHandle( humanReadableCode.c_str() );
	MLOGPK( DBG, this ) << "PEAK handle for vectorString[1]= " << vectorString[1]
	      << " is code= 0x" <<hex <<  m_canObjHandler << dec
		  << " human readable code= " << humanReadableCode << std::endl;


	if (strcmp(parameters.c_str(), "Unspecified") != 0)
	{
		//numPar = sscanf_s(canpars, "%d %d %d %d %d %d", &parametersBaudRate, &parametersTseg1, &parametersTseg2, &parametersSjw, &parametersNoSamp, &parametersSyncmode);
		//Get the can object handler
		m_canObjHandler = getHandle(vectorString[1].c_str());
		//Process the baudRate if needed
		if (m_CanParameters.m_iNumberOfDetectedParameters == 1)
		{
			MLOGPK(DBG, this) << " m_CanParameters.m_lBaudRate= " << m_CanParameters.m_lBaudRate << std::endl;
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
	MLOGPK(DBG, this) << " m_baudRate= " << m_baudRate << std::endl;

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
	 */
	MLOGPK(TRC, this) << "calling CAN_Initialize";
	TPCANStatus tpcanStatus = CAN_Initialize(m_canObjHandler, m_baudRate );
	MLOGPK(TRC, this) << "CAN_Initialize returns " << (int) tpcanStatus;

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
		status = CAN_Reset(m_canObjHandler);
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
		tpcanStatus = CAN_Write(m_canObjHandler, &tpcanMessage);
		if (tpcanStatus != PCAN_ERROR_OK) {
			return sendErrorCode(tpcanStatus);
		}
		m_statistics.onTransmit( tpcanMessage.LEN );
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
	TPCANStatus tpcanStatus = CAN_Write(m_canObjHandler, &tpcanMessage);
	return sendErrorCode(tpcanStatus);
}

bool PKCanScan::getErrorMessage(long error, char **message)
{
	char tmp[300];
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

