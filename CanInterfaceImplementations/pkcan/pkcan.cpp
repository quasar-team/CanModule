// This is the main DLL file.

#include "pkcan.h"
#include <time.h>
#include <string.h>
#include "CanModuleUtils.h"

#include <LogIt.h>
using namespace std;

bool  initLibarary =  false;
extern "C" __declspec(dllexport) CCanAccess *getCanBusAccess()
{
	CCanAccess *cc;
	cc = new PKCanScan;
	return cc;
}

PKCanScan::PKCanScan()
{
	m_statistics.beginNewRun();
	m_busStatus = 0;
}

PKCanScan::~PKCanScan()
{
	m_CanScanThreadShutdownFlag = false;
//	CloseHandle(m_ReadEvent);
	CAN_Uninitialize(m_canObjHandler);
}

DWORD WINAPI PKCanScan::CanScanControlThread(LPVOID pCanScan)
{
	PKCanScan *pkCanScanPointer = reinterpret_cast<PKCanScan *>(pCanScan);
	TPCANHandle tpcanHandler = pkCanScanPointer->m_canObjHandler;
	MLOG(DBG,pkCanScanPointer) << "CanScanControlThread Started. m_CanScanThreadShutdownFlag = [" << pkCanScanPointer->m_CanScanThreadShutdownFlag <<"]";
	while ( pkCanScanPointer->m_CanScanThreadShutdownFlag ) {
		TPCANMsg tpcanMessage;
		TPCANTimestamp tpcanTimestamp;
		TPCANStatus tpcanStatus = CAN_Read(tpcanHandler,&tpcanMessage,&tpcanTimestamp);
		if (tpcanStatus == PCAN_ERROR_OK)
		{
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
		}
		else
		{
			if (tpcanStatus & PCAN_ERROR_QRCVEMPTY)
			{
				continue;
			}
			pkCanScanPointer->sendErrorCode(tpcanStatus);
			if (tpcanStatus | PCAN_ERROR_ANYBUSERR)
			{
				CAN_Initialize(tpcanHandler,pkCanScanPointer->m_baudRate);
				Sleep(100);
			}
		}
	}
	ExitThread(0);
	return 0;
}

bool PKCanScan::createBus(const string name ,const string parameters )
{
	m_sBusName = name;
	//We configure the canboard
	if (!configureCanboard(name,parameters))
	{//If we can't initialise the canboard, the thread is not started at all
		return false;
	}
	// Otherwise, we initialise the Scan thread
	CAN_FilterMessages(m_canObjHandler,0,0x7FF,PCAN_MESSAGE_STANDARD);
	m_hCanScanThread = CreateThread(NULL, 0, CanScanControlThread, this, 0, &m_idCanScanThread);
	if (NULL == m_hCanScanThread)
	{
		DebugBreak();
		return false;
	}
	return true;
}

bool PKCanScan::configureCanboard(const string name,const string parameters)
{
	m_sBusName = name;
	m_baudRate = PCAN_BAUD_125K;

	//unsigned int parametersTseg1 = 0;
	//unsigned int parametersTseg2 = 0;
	//unsigned int parametersSjw = 0;
	//unsigned int parametersNoSamp = 0;
	//unsigned int parametersSyncmode = 0;
	//long parametersBaudRate;
	//int	numPar;
	//Process the parameters
	vector<string> vectorString;
	vectorString = parcerNameAndPar(name, parameters);
	//const char *canpars = parameters.c_str();
	if (strcmp(parameters.c_str(), "Unspecified") != 0)
	{
		//numPar = sscanf_s(canpars, "%d %d %d %d %d %d", &parametersBaudRate, &parametersTseg1, &parametersTseg2, &parametersSjw, &parametersNoSamp, &parametersSyncmode);
		//Get the can object handler
		m_canObjHandler = getHandle(vectorString[1].c_str());
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
		}
		else
		{
			if (m_CanParameters.m_iNumberOfDetectedParameters != 0)
			{
				m_baudRate = m_CanParameters.m_lBaudRate;
			}
			else
			{
				MLOG(ERR, this) << "Error while parsing parameters: this syntax is incorrect: [" << parameters << "]";
				return false;
			}
		}
	}
	else
	{
		MLOG(DBG, this) << "Unspecified parameters, default values will be used.";
	}
	//Initialize the canboard
	TPCANStatus tpcanStatus = CAN_Initialize(m_canObjHandler, m_baudRate,256,3);
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

bool PKCanScan::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
{
	TPCANStatus tpcanStatus;
	TPCANMsg tpcanMessage;
	tpcanMessage.ID = cobID;
	tpcanMessage.MSGTYPE = PCAN_MESSAGE_STANDARD;
	if(rtr)
	{
		tpcanMessage.MSGTYPE = PCAN_MESSAGE_RTR;
	}
	int lengthOfUnsentData, lengthToBeSent;
	lengthOfUnsentData = len;
	do
	{
		//In every iteration of this loop a piece of message is sent of maximun 8 chars
		//To keep track of how much message is left we use vars lengthOfUnsentData and lengthToBeSent
		if (lengthOfUnsentData > 8)
		{
			lengthToBeSent = 8;
			lengthOfUnsentData = lengthOfUnsentData - 8;
		}
		else
		{
			lengthToBeSent = lengthOfUnsentData;
		}
		tpcanMessage.LEN = lengthToBeSent;

		memcpy(tpcanMessage.DATA,message,lengthToBeSent);
		tpcanStatus = CAN_Write(m_canObjHandler, &tpcanMessage);
		if (tpcanStatus != PCAN_ERROR_OK)
		{
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
	for (unsigned int j=0; j < 4; j++)
	{
		tmpName[j] = toupper(name[j]);
	}
	for (int i = 0; i < chn; i++)
	{
		if (strcmp(channelNameArray[i],tmpName) == 0)
		{
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

