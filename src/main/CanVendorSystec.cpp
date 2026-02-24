/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * STCanScap.cpp
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

#include "CanVendorSystec.h"

#include <time.h>
#include <vector>
#include <LogIt.h>
#include <iomanip>
#include <string>
#include <algorithm>

CanVendorSystec::CanVendorSystec(const CanDeviceArguments& args)
    : CanDevice("systec", args) {
  if (!args.config.bus_name.has_value()) {
    throw std::invalid_argument("Missing required configuration parameters");
  }

  // TODO trim possible can prefix use hardcoded value
  int handleNumber = std::stoi(args.config.bus_name.value());
  m_moduleNumber = handleNumber / 2;
  m_channelNumber = handleNumber % 2;
}

/**
 * We create and fill initializationParameters, to pass it to openCanPort
 */
 /* static */ tUcanInitCanParam createInitializationParameters( unsigned int baudRate ){
	tUcanInitCanParam   initializationParameters;
    initializationParameters.m_dwSize = sizeof(initializationParameters);           // size of this struct
    initializationParameters.m_bMode = kUcanModeNormal;              // normal operation mode
    initializationParameters.m_bBTR0 = HIBYTE( baudRate );              // baudrate
    initializationParameters.m_bBTR1 = LOBYTE( baudRate );
    initializationParameters.m_bOCR = 0x1A;                         // standard output
    initializationParameters.m_dwAMR = USBCAN_AMR_ALL;               // receive all CAN messages
    initializationParameters.m_dwACR = USBCAN_ACR_ALL;
    initializationParameters.m_dwBaudrate = USBCAN_BAUDEX_USE_BTR01;
    initializationParameters.m_wNrOfRxBufferEntries = USBCAN_DEFAULT_BUFFER_ENTRIES;
    initializationParameters.m_wNrOfTxBufferEntries = USBCAN_DEFAULT_BUFFER_ENTRIES;

	return ( initializationParameters );
}

CanReturnCode CanVendorSystec::init_can_port() {
	BYTE systecCallReturn = USBCAN_SUCCESSFUL;
	tUcanHandle		canModuleHandle;

	// check if USB-CANmodul already is initialized
  auto pos = m_handleMap.find(m_moduleNumber);
  if (pos == m_handleMap.end()) { // module not in use
  	systecCallReturn = ::UcanInitHardwareEx(&canModuleHandle, m_moduleNumber, 0, 0);
		if (systecCallReturn != USBCAN_SUCCESSFUL ) 	{
			LOG(Log::ERR, CanLogIt::h()) << "UcanInitHardwareEx, return code = [ 0x" << std::hex << (int) systecCallReturn << std::dec << "]";
			::UcanDeinitHardware(canModuleHandle);
			return CanReturnCode::unknown_open_error;
    }
    m_handleMap[m_moduleNumber] = canModuleHandle;
  } else { // find existing handle of module
    canModuleHandle = pos->second;
	 	LOG(Log::WRN, CanLogIt::h()) << "trying to open a can port which is in use, reuse handle, skipping UCanDeinitHardware";
  }

  unsigned int baudRate = USBCAN_BAUD_125kBit;
  switch (args().config.bitrate.value_or(0)) {
    case 50000: baudRate = USBCAN_BAUD_50kBit; break;
    case 100000: baudRate = USBCAN_BAUD_100kBit; break;
    case 125000: baudRate = USBCAN_BAUD_125kBit; break;
    case 250000: baudRate = USBCAN_BAUD_250kBit; break;
    case 500000: baudRate = USBCAN_BAUD_500kBit; break;
    case 1000000: baudRate = USBCAN_BAUD_1MBit; break;
    default: {
	 	  LOG(Log::WRN, CanLogIt::h()) << "baud rate illegal, taking default 125000 [" << baudRate << "]";
    }
  }
  auto initializationParameters = createInitializationParameters(baudRate);

  systecCallReturn = ::UcanInitCanEx2(canModuleHandle, m_channelNumber, &initializationParameters);
  if ( systecCallReturn != USBCAN_SUCCESSFUL )	{
    LOG(Log::ERR, CanLogIt::h()) << "UcanInitCanEx2, return code = [ 0x" << std::hex << (int) systecCallReturn << std::dec << "]";
    ::UcanDeinitCanEx(canModuleHandle, m_channelNumber);
    return CanReturnCode::unknown_open_error;
  }

  m_UcanHandle = canModuleHandle;
  LOG(Log::INF, CanLogIt::h()) << "Successfully opened CAN port on module " << m_moduleNumber << ", channel " << m_channelNumber;
  return CanReturnCode::success;
}

CanReturnCode CanVendorSystec::vendor_open() noexcept {

  auto returnCode = init_can_port();

  // TODO set time since opened equivalent...
  // m_statistics.setTimeSinceOpened();

	// After the canboard is configured and started, we start the scan control thread
	m_hReceiveThread = CreateThread(NULL, 0, SystecRxThread, this, 0, &m_idReceiveThread);

	if (NULL == m_hReceiveThread) {
	  LOG(Log::ERR, CanLogIt::h()) << "Error creating the canScanControl thread.";
		return CanReturnCode::unknown_open_error;
	}

  return returnCode;
}

CanReturnCode CanVendorSystec::vendor_close() noexcept {
  // TODO what if the return code is not success?
  m_CanScanThreadShutdownFlag = false;
	DWORD result = WaitForSingleObject(m_hReceiveThread, INFINITE); 	//Shut down can scan thread
	UcanDeinitCanEx (m_UcanHandle, (BYTE)m_channelNumber);
	LOG(Log::DBG, CanLogIt::h()) << __FUNCTION__ << " closed successfully";
  return CanReturnCode::success;
};

CanReturnCode CanVendorSystec::vendor_send(const CanFrame& frame) noexcept {
  // bool CanVendorSystec::sendMessage(short cobID, unsigned char len, unsigned char *message, bool rtr)
  bool rtr = frame.is_remote_request();
  uint32_t len = frame.length();
  char *message = frame.message().data();
  short cobID = frame.id(); // is this the same as cobid? i don't know...
  
  // TODO we can't just use message as it's now a vector of chars not a char*
  
  LOG(Log::DBG, CanLogIt::h()) << "Sending message: [" << ( message == 0  ? "" : (const char *) message) << "], cobID: [" << cobID << "], Message Length: [" << static_cast<int>(len) << "]";

	tCanMsgStruct canMsgToBeSent;
	BYTE Status;

	canMsgToBeSent.m_dwID = cobID;
	canMsgToBeSent.m_bDLC = len;
	canMsgToBeSent.m_bFF = 0;
	if (rtr) {
		canMsgToBeSent.m_bFF = USBCAN_MSG_FF_RTR;
	}
	int  messageLengthToBeProcessed;
	//If there is more than 8 characters to process, we process 8 of them in this iteration of the loop
	if (len > 8) {
		messageLengthToBeProcessed = 8;
		LOG(Log::DBG, CanLogIt::h()) << "The length is more then 8 bytes, adjust to 8, ignore >8. len= " << len;
	} else {
		//Otherwise if there is less than 8 characters to process, we process all of them in this iteration of the loop
		messageLengthToBeProcessed = len;
		if (len < 8) {
			LOG(Log::DBG, CanLogIt::h())<< "The length is less then 8 bytes, process only. len= " << len;
		}
	}
	canMsgToBeSent.m_bDLC = messageLengthToBeProcessed;
	memcpy(canMsgToBeSent.m_bData, message, messageLengthToBeProcessed);
	//	MLOG(TRC,this) << "Channel Number: [" << m_channelNumber << "], cobID: [" << canMsgToBeSent.m_dwID << "], Message Length: [" << static_cast<int>(canMsgToBeSent.m_bDLC) << "]";
	Status = UcanWriteCanMsgEx(m_UcanHandle, m_channelNumber, &canMsgToBeSent, NULL);
	if (Status != USBCAN_SUCCESSFUL) {
		LOG(Log::ERR, CanLogIt::h()) << "There was a problem when sending a message.";

    // for now, just always reconnect on a failed send.
    vendor_close();
    vendor_open();

    return CanReturnCode::unknown_send_error;
	} else {
		// m_statistics.onTransmit( canMsgToBeSent.m_bDLC );
		// m_statistics.setTimeSinceTransmitted();
    return CanReturnCode::success;
	}
	// return sendErrorCode(Status);
};

CanDiagnostics CanVendorSystec::vendor_diagnostics() noexcept {

  // TODO we can read the operating mode, either kUcanModeNormal, ListenOnly or TxEcho
  CanDiagnostics diagnostics{};
  tStatusStruct status;
  // TODO check return code of these functions...
  UcanGetStatusEx(m_UcanHandle, m_channelNumber, &status);
  WORD can_status = status.m_wCanStatus;
  switch (can_status) {
    case USBCAN_CANERR_OK:
      diagnostics.state = "USBCAN_CANERR_OK"; break;
    case USBCAN_CANERR_XMTFULL:
      diagnostics.state = "USBCAN_CANERR_XMTFULL"; break;
    case USBCAN_CANERR_OVERRUN:
      diagnostics.state = "USBCAN_CANERR_OVERRUN"; break;
    case USBCAN_CANERR_BUSLIGHT:
      diagnostics.state = "USBCAN_CANERR_BUSLIGHT"; break;
    case USBCAN_CANERR_BUSHEAVY:
      diagnostics.state = "USBCAN_CANERR_BUSHEAVY"; break;
    case USBCAN_CANERR_BUSOFF:
      diagnostics.state = "USBCAN_CANERR_BUSOFF"; break;
    case USBCAN_CANERR_QOVERRUN:
      diagnostics.state = "USBCAN_CANERR_QOVERRUN"; break;
    case USBCAN_CANERR_QXMTFULL:
      diagnostics.state = "USBCAN_CANERR_QXMTFULL"; break;
    case USBCAN_CANERR_REGTEST:
      diagnostics.state = "USBCAN_CANERR_REGTEST"; break;
    case USBCAN_CANERR_TXMSGLOST:
      diagnostics.state = "USBCAN_CANERR_TXMSGLOST"; break;
  }

  tUcanMsgCountInfo msg_count_info;
  UcanGetMsgCountInfoEx(m_UcanHandle, m_channelNumber, &msg_count_info);
  diagnostics.tx = msg_count_info.m_wSentMsgCount;
  diagnostics.rx = msg_count_info.m_wRecvdMsgCount;

  DWORD tx_error, rx_error;
  UcanGetCanErrorCounter(m_UcanHandle, m_channelNumber, &tx_error, &rx_error);
  diagnostics.tx_error = tx_error;
  diagnostics.rx_error = rx_error;

  tUcanHardwareInfo hw_info;
  if (UcanGetHardwareInfo(m_UcanHandle, &hw_info) != USBCAN_SUCCESSFUL)
    diagnostics.mode = "OFFLINE";
  else switch (hw_info.m_bMode) {
    case kUcanModeNormal:
      diagnostics.mode = "NORMAL"; break;
    case kUcanModeListenOnly:
      diagnostics.mode = "LISTEN_ONLY"; break;
    case kUcanModeTxEcho:
      diagnostics.mode = "LOOPBACK"; break;
  }

  // DWORD module_time;
  // UcanGetModuleTime(m_UcanHandle, &module_time);

  return diagnostics;
};

std::string UsbCanGetErrorText( long errCode ); // forward declaration

/**
 * thread to handle reception of Can messages from the systec device
 */
DWORD WINAPI CanVendorSystec::SystecRxThread(LPVOID pCanVendorSystec)
{
	BYTE status;
	tCanMsgStruct readCanMessage;
	CanVendorSystec *vendorPointer = reinterpret_cast<CanVendorSystec*>(pCanVendorSystec);
	LOG(Log::DBG, CanLogIt::h()) << "SystecRxThread Started. m_CanScanThreadShutdownFlag = [" << vendorPointer->m_CanScanThreadShutdownFlag <<"]";
	while (vendorPointer->m_CanScanThreadShutdownFlag) {
		status = UcanReadCanMsgEx(vendorPointer->m_UcanHandle, (BYTE *)&vendorPointer->m_channelNumber, &readCanMessage, NULL);
    switch (status) {
      case USBCAN_WARN_SYS_RXOVERRUN: // fallthrough intended for warnings
      case USBCAN_WARN_DLL_RXOVERRUN:
      case USBCAN_WARN_FW_RXOVERRUN:
        LOG(Log::WRN, CanLogIt::h()) << UsbCanGetErrorText(status);
      case USBCAN_SUCCESSFUL: {
        if (readCanMessage.m_bFF & USBCAN_MSG_FF_RTR) break;
        // canMsgCopy.c_time = convertTimepointToTimeval(currentTimeTimeval());
        std::vector<char> data(8);
        std::copy(readCanMessage.m_bData, readCanMessage.m_bData + 8, data.begin());
        // id, data, flags
        CanFrame canMsgCopy(readCanMessage.m_dwID, data, readCanMessage.m_bFF);
      // TODO the readCanMessage contains a DWORD m_dwTime "receipt time in ms"
        vendorPointer->received(canMsgCopy);
        // vendorPointer->m_statistics.onReceive( readCanMessage.m_bDLC );
        // vendorPointer->m_statistics.setTimeSinceReceived();

        // we can reset the reconnectionTimeout here, since we have received a message
        // vendorPointer->resetTimeoutOnReception();
        break;
      }
      case USBCAN_WARN_NODATA:
        LOG(Log::TRC, CanLogIt::h()) << UsbCanGetErrorText(status);
        // TODO is it correct to sleep here?
        // Sleep(100); // ms
        break;
        default: // errors
        // USBCAN_ERR_MAXINSTANCES, USBCAN_ERR_ILLHANDLE, USBCAN_ERR_CANNOTINIT,
        // USBCAN_ERR_ILLPARAM, USBCAN_ERR_ILLHW, USBCAN_ERR_ILLCHANNEL
        // TODO should we raise some error state here?
        LOG(Log::ERR, CanLogIt::h()) << UsbCanGetErrorText(status);
        break;
		}
  }

	ExitThread(0);
	return 0;
}

/**
 * error text specific to STcan according to table24
 * I am just copying the whole descriptions from the doc, verbatim, wtf.
 * you get some shakespeare from it.
 */
std::string UsbCanGetErrorText( long errCode ){
	switch( errCode ){
	case USBCAN_SUCCESSFUL: return("success");

	case USBCAN_ERR_RESOURCE: return ("This error code returns if one resource could not be generated. In this\
	case the term resource means memory and handles provided by the	Windows OS");

	case USBCAN_ERR_MAXMODULES: return("An application has tried to open more than 64 USB-CANmodul devices.\
	The standard version of the DLL only supports up to 64 USB-CANmodul\
	devices at the same time. This error also appears if several applications\
	try to access more than 64 USB-CANmodul devices. For example,\
	application 1 has opened 60 modules, application 2 has opened 4\
	modules and application 3 wants to open a module. Application 3\
	receives this error code.");

	case USBCAN_ERR_HWINUSE: return("An application tries to initialize an USB-CANmodul with the given device\
	number. If this module has already been initialized by its own or by\
	another application, this error code is returned.");

	case USBCAN_ERR_ILLVERSION: return("This error code returns if the firmware version of the USB-CANmodul is\
	not compatible to the software version of the DLL. In this case, install\
	the latest driver for the USB-CANmodul. Furthermore make sure that\
	the latest firmware version is programmed to the USB-CANmodul.");

	case USBCAN_ERR_ILLHW: return("This error code returns if an USB-CANmodul with the given device\
	number is not found. If the function UcanInitHardware() or\
	UcanInitHardwareEx() has been called with the device number\
	USBCAN_ANY_MODULE, and the error code appears, it indicates that\
	no module is connected to the PC or all connected modules are already\
	in use.");

	case USBCAN_ERR_ILLHANDLE: return("This error code returns if a function received an incorrect USBCAN\
	handle. The function first checks which USB-CANmodul is related to this\
	handle. This error occurs if no device belongs this handle.");

	case USBCAN_ERR_ILLPARAM: return("This error code returns if a wrong parameter is passed to the function.\
	For example, the value NULL has been passed to a pointer variable\
	instead of a valid address.");

	case USBCAN_ERR_BUSY: return("This error code occurs if several threads are accessing an\
	USB-CANmodul within a single application. After the other threads have\
	finished their tasks, the function may be called again.");

	case USBCAN_ERR_TIMEOUT: return("This error code occurs if the function transmits a command to the\
	USB-CANmodul but no reply is returned. To solve this problem, close\
	the application, disconnect the USB-CANmodul, and connect it again.");

	case USBCAN_ERR_IOFAILED: return("This error code occurs if the communication to the kernel driver was\
	interrupted. This happens, for example, if the USB-CANmodul is\
	disconnected during transferring data or commands to the\
	USB-CANmodul.");

	case USBCAN_ERR_DLL_TXFULL: return("The function UcanWriteCanMsg() or UcanWriteCanMsgEx() first checks\
	if the transmit buffer within the DLL has enough capacity to store new\
	CAN messages. If the buffer is full, this error code returns. The CAN\
	message passed to these functions will not be written into the transmit\
	buffer in order to protect other CAN messages against overwriting. The\
	size of the transmit buffer is configurable (refer to function\
	UcanInitCanEx() and structure tUcanInitCanParam).");

	case USBCAN_ERR_MAXINSTANCES: return("A maximum amount of 64 applications are able to have access to the\
	DLL. If more applications attempting to access to the DLL, this error\
	code is returned. In this case, it is not possible to use an\
	USB-CANmodul by this application.");

	case USBCAN_ERR_CANNOTINIT: return("This error code returns if an application tries to call an API function\
	which only can be called in software state CAN_INIT but the current\
	software is still in state HW_INIT. Refer to section 4.3.1 and Table 11 for\
	detailed information.");

	case USBCAN_ERR_DISCONNECT: return("This error code occurs if an API function was called for an\
	USB-CANmodul that was plugged-off from the computer recently.");

	case USBCAN_ERR_ILLCHANNEL: return("This error code is returned if an extended function of the DLL is called\
	with parameter bChannel_p = USBCAN_CHANNEL_CH1, but a single-channel USB-CANmodul was used.");

	case USBCAN_ERR_ILLHWTYPE: return("This error code occurs if an extended function of the DLL was called for\
	a hardware which does not support the feature.");

	case USBCAN_ERRCMD_NOTEQU: return("This error code occurs during communication between the PC and an\
	USB-CANmodul. The PC sends a command to the USB-CANmodul,\
	then the module executes the command and returns a response to the\
	PC. This error code returns if the reply does not correspond to the command.");

	case USBCAN_ERRCMD_REGTST: return("The software tests the CAN controller on the USB-CANmodul when the\
	CAN interface is initialized. Several registers of the CAN controller are\
	checked. This error code returns if an error appears during this register test.");

	case USBCAN_ERRCMD_ILLCMD: return("This error code returns if the USB-CANmodul receives a non-defined\
	command. This error represents a version conflict between the firmware in the USB-CANmodul and the DLL.");

	case USBCAN_ERRCMD_EEPROM: return("The USB-CANmodul has a built-in EEPROM. This EEPROM contains\
	several configurations, e.g. the device number and the serial number. If\
	an error occurs while reading these values, this error code is returned.");

	case USBCAN_ERRCMD_ILLBDR: return("The USB-CANmodul has been initialized with an invalid baud rate (refer\
	to section 4.3.4).");

	case USBCAN_ERRCMD_NOTINIT: return("It was tried to access a CAN-channel of a multi-channel\
	USB-CANmodul that was not initialized.");

	case USBCAN_ERRCMD_ALREADYINIT: return("The accessed CAN-channel of a multi-channel USB-CANmodul was\
	already initialized");

	case USBCAN_ERRCMD_ILLSUBCMD: return("An internal error occurred within the DLL. In this case an unknown sub-\
	command was called instead of a main command (e.g. for the cyclic CAN message-feature).");

	case USBCAN_ERRCMD_ILLIDX: return("An internal error occurred within the DLL. In this case an invalid index\
	for a list was delivered to the firmware (e.g. for the cyclic CAN message-feature).");

	case USBCAN_ERRCMD_RUNNING: return("The caller tries to define a new list of cyclic CAN messages but this\
	feature was already started. For defining a new list, it is necessary to stop the feature beforehand.");

	case USBCAN_WARN_NODATA: return("If the function UcanReadCanMsg() or UcanReadCanMsgEx() returns\
	with this warning, it is an indication that the receive buffer contains no CAN messages.");

	case USBCAN_WARN_SYS_RXOVERRUN: return("This is returned by UcanReadCanMsg() or UcanReadCanMsgEx() if the\
	receive buffer within the kernel driver runs over. The function\
	nevertheless returns a valid CAN message. It also indicates that at least\
	one CAN message are lost. However, it does not indicate the position of the lost CAN messages.");

	case USBCAN_WARN_DLL_RXOVERRUN: return("The DLL automatically requests CAN messages from the\
	USB-CANmodul and stores the messages into a buffer of the DLL. If\
	more CAN messages are received than the DLL buffer size allows, this\
	error code returns and CAN messages are lost. However, it does not\
	indicate the position of the lost CAN messages. The size of the receive\
	buffer is configurable (refer to function UcanInitCanEx() and structure\
	tUcanInitCanParam).");

	case USBCAN_WARN_FW_TXOVERRUN: return("This warning is returned by function UcanWriteCanMsg() or\
	UcanWriteCanMsgEx() if flag USBCAN_CANERR_QXMTFULL is set in\
	the CAN driver status. However, the transmit CAN message could be\
	stored to the DLL transmit buffer. This warning indicates that at least\
	one transmit CAN message got lost in the device firmware layer. This\
	warning does not indicate the position of the lost CAN message.");

	case USBCAN_WARN_FW_RXOVERRUN: return("This warning is returned by function UcanWriteCanMsg() or\
	UcanWriteCanMsgEx() if flag USBCAN_CANERR_QOVERRUN or flag\
	USBCAN_CANERR_OVERRUN are set in the CAN driver status. The\
	function has returned with a valid CAN message. This warning indicates\
	that at least one received CAN message got lost in the firmware layer.\
	This warning does not indicate the position of the lost CAN message.");

	case USBCAN_WARN_NULL_PTR: return("This warning is returned by functions UcanInitHwConnectControl() or\
	UcanInitHwConnectControlEx() if a NULL pointer was passed as callback function address.");

	case USBCAN_WARN_TXLIMIT: return("This warning is returned by the function UcanWriteCanMsgEx() if it was\
	called to transmit more than one CAN message, but a part of them\
	could not be stored to the transmit buffer within the DLL (because the\
	buffer is full). The returned variable addressed by the parameter\
	pdwCount_p indicates the number of CAN messages which are stored\
	successfully to the transmit buffer.");

	default: return("unknown error code");
	}
}