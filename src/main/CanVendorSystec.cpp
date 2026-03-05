#include "CanVendorSystec.h"

#include <algorithm>
#include <time.h>
#include <LogIt.h>
#include <iomanip>
#include <vector>

std::mutex CanVendorSystec::m_handles_lock;
std::unordered_map<int, tUcanHandle> CanVendorSystec::m_module_to_handle_map;
std::unordered_map<int, CanVendorSystec*> CanVendorSystec::m_port_to_vendor_map;

// Callback registered per-module to handle receive events
void systec_receive(tUcanHandle UcanHandle_p, DWORD bEvent_p, BYTE bChannel_p, void* pArg_p) {
  if (bEvent_p == USBCAN_EVENT_RECEIVE) {
    int module_number = *(reinterpret_cast<int*>(pArg_p));
    int port_number = 2 * module_number + bChannel_p;
    CanVendorSystec *vendorPtr = CanVendorSystec::m_port_to_vendor_map[port_number];
    if (!vendorPtr) return; // [] returns nullptr if not found in map
    ++(vendorPtr->m_queued_reads);
  }
}

CanVendorSystec::CanVendorSystec(const CanDeviceArguments& args)
    : CanDevice("systec", args), m_queued_reads{0} {
  if (!args.config.bus_name.has_value()) {
    throw std::invalid_argument("Missing required configuration parameters");
  }

  // TODO trim possible can prefix
  m_port_number = std::stoi(args.config.bus_name.value());
  m_module_number = m_port_number / 2;
  m_channel_number = m_port_number % 2;
  m_port_to_vendor_map[m_port_number] = this;
}

// TODO should we make this noexcept? how can we guarantee that?
CanReturnCode CanVendorSystec::init_can_port() {
  BYTE systec_call_return = USBCAN_SUCCESSFUL;
  tUcanHandle		can_module_handle;

  unsigned int baud_rate = USBCAN_BAUD_125kBit;
  switch (args().config.bitrate.value_or(0)) {
    case 50000:   baud_rate = USBCAN_BAUD_50kBit;  break;
    case 100000:  baud_rate = USBCAN_BAUD_100kBit; break;
    case 125000:  baud_rate = USBCAN_BAUD_125kBit; break;
    case 250000:  baud_rate = USBCAN_BAUD_250kBit; break;
    case 500000:  baud_rate = USBCAN_BAUD_500kBit; break;
    case 1000000: baud_rate = USBCAN_BAUD_1MBit;   break;
    default: {
      LOG(Log::WRN, CanLogIt::h()) << "baud rate illegal, taking default 125000 [" << baud_rate << "]";
    }
  }

  tUcanInitCanParam initialization_parameters;
  initialization_parameters.m_dwSize = sizeof(initialization_parameters); // size of this struct
  initialization_parameters.m_bMode = kUcanModeNormal; // normal operation mode
  initialization_parameters.m_bBTR0 = HIBYTE( baud_rate );  // baudrate
  initialization_parameters.m_bBTR1 = LOBYTE( baud_rate );
  initialization_parameters.m_bOCR = 0x1A; // standard output
  initialization_parameters.m_dwAMR = USBCAN_AMR_ALL;  // receive all CAN messages
  initialization_parameters.m_dwACR = USBCAN_ACR_ALL;
  initialization_parameters.m_dwBaudrate = USBCAN_BAUDEX_USE_BTR01;
  initialization_parameters.m_wNrOfRxBufferEntries = USBCAN_DEFAULT_BUFFER_ENTRIES;
  initialization_parameters.m_wNrOfTxBufferEntries = USBCAN_DEFAULT_BUFFER_ENTRIES;

  // check if USB-CANmodul is already initialized
  std::lock_guard<std::mutex> guard(CanVendorSystec::m_handles_lock);
  auto mapping = m_module_to_handle_map.find(m_module_number);
  if (mapping == m_module_to_handle_map.end()) { // module not in use
    systec_call_return = ::UcanInitHardwareEx(&can_module_handle, m_module_number, systec_receive, (void*) &m_module_number);
    if (systec_call_return != USBCAN_SUCCESSFUL ) 	{
      LOG(Log::ERR, CanLogIt::h()) << "UcanInitHardwareEx, return code = [ 0x" << std::hex << (int) systec_call_return << std::dec << "]";
      ::UcanDeinitHardware(can_module_handle);
      return CanReturnCode::unknown_open_error;
    }
    LOG(Log::INF, CanLogIt::h()) << "Initialised hardware for Systec module " << m_module_number <<  " with handle " << (int) can_module_handle;
    m_module_to_handle_map[m_module_number] = can_module_handle;
  } else { // find existing handle of module
    can_module_handle = mapping->second;
    LOG(Log::WRN, CanLogIt::h()) << "trying to open a can port which is in use, reuse handle, skipping UCanInitHardware";
  }

  // TODO handle error code for reset...
  // also investigate the minimum amount of things to reset to restore good state
  UcanResetCanEx(can_module_handle, (BYTE) m_channel_number, (DWORD) 0);
  m_port_to_vendor_map[m_port_number] = this;

  systec_call_return = ::UcanInitCanEx2(can_module_handle, m_channel_number, &initialization_parameters);
  if ( systec_call_return != USBCAN_SUCCESSFUL )	{
    LOG(Log::ERR, CanLogIt::h()) << "UcanInitCanEx2, return code = [ 0x" << std::hex << (int) systec_call_return << std::dec << "]";
    ::UcanDeinitCanEx(can_module_handle, m_channel_number);
    return CanReturnCode::unknown_open_error;
  }

  LOG(Log::INF, CanLogIt::h()) << "Successfully opened CAN port on module " << m_module_number << ", channel " << m_channel_number;
  return CanReturnCode::success;
}

CanReturnCode CanVendorSystec::vendor_open() noexcept {

  auto return_code = init_can_port();
  if (return_code != CanReturnCode::success) return return_code;

  try {
    m_receive_thread_flag = true;
    m_SystecRxThread = std::thread(&CanVendorSystec::SystecRxThread, this);
  } catch(...) {
    return_code = CanReturnCode::internal_api_error;
  }

  return return_code;
}

CanReturnCode CanVendorSystec::vendor_close() noexcept {
  try {
    m_receive_thread_flag = false;
    std::lock_guard<std::mutex> guard(CanVendorSystec::m_handles_lock);
    m_port_to_vendor_map.erase(m_port_number);
    if (m_SystecRxThread.joinable()) m_SystecRxThread.join();

    auto handle = get_module_handle();

    auto return_code = UcanDeinitCanEx(handle, (BYTE) m_channel_number);
    m_module_to_handle_map.erase(m_module_number);
    if (return_code != USBCAN_SUCCESSFUL) return CanReturnCode::unknown_close_error;

    int opposite_channel = 2 * m_module_number + (1 - m_channel_number);
    if (!m_port_to_vendor_map[opposite_channel]) {
      // de init hardware if neither channel on the module are in use
      // e.g. if channel 0, we need to check if channel 1 is in use and vice versa
      // TODO how does this work if UcanDeinitCanEx above fails?
      auto return_code_hw = UcanDeinitHardware(handle);
      if (return_code_hw != USBCAN_SUCCESSFUL) return CanReturnCode::unknown_close_error;
    }
  } catch (...) {
    return CanReturnCode::internal_api_error;
  }
  return CanReturnCode::success;
};

CanReturnCode CanVendorSystec::vendor_send(const CanFrame& frame) noexcept {
  uint32_t len = frame.length();
  std::vector<char> message = frame.message();

  tCanMsgStruct can_msg_to_send;
  BYTE Status;

  can_msg_to_send.m_dwID = frame.id();
  can_msg_to_send.m_bDLC = len;
  can_msg_to_send.m_bFF = 0;
  if (frame.is_remote_request()) {
      can_msg_to_send.m_bFF = USBCAN_MSG_FF_RTR;
  }
  int message_length_to_process;
  //If there is more than 8 characters to process, we process 8 of them in this iteration of the loop
  if (len > 8) {
      message_length_to_process = 8;
      LOG(Log::DBG, CanLogIt::h()) << "The length is more than 8 bytes, adjust to 8, ignore > 8. len = " << len;
  } else {
      //Otherwise if there is less than 8 characters to process, we process all of them in this iteration of the loop
      message_length_to_process = len;
      if (len < 8) {
          LOG(Log::DBG, CanLogIt::h())<< "The length is less than 8 bytes, process only. len = " << len;
      }
  }
  can_msg_to_send.m_bDLC = message_length_to_process;
  if (message_length_to_process)
    std::copy(message.begin(), message.begin() + message_length_to_process, can_msg_to_send.m_bData);

  Status = UcanWriteCanMsgEx(get_module_handle(), m_channel_number, &can_msg_to_send, NULL);
  if (Status != USBCAN_SUCCESSFUL) {
    LOG(Log::ERR, CanLogIt::h()) << "There was a problem when sending a message: "
      << UsbCanGetErrorText(Status);

    // for now, just always reconnect on a failed send.
    auto close_code = close();
    if (close_code != CanReturnCode::success) return close_code;

    auto open_code = open();
    if (open_code != CanReturnCode::success) return open_code;

    switch (Status) {
      case USBCAN_ERR_CANNOTINIT:
      case USBCAN_ERR_ILLHANDLE: return CanReturnCode::disconnected;
      case USBCAN_ERR_DLL_TXFULL: return CanReturnCode::tx_buffer_overflow;
      case USBCAN_ERR_MAXINSTANCES: return CanReturnCode::too_many_connections;
      case USBCAN_ERR_ILLPARAM:
      case USBCAN_ERR_ILLHW:
      case USBCAN_ERR_ILLCHANNEL:
      case USBCAN_WARN_TXLIMIT:
      case USBCAN_WARN_FW_TXOVERRUN:
      default: return CanReturnCode::unknown_send_error;
    }
  }
  return CanReturnCode::success;
};

CanDiagnostics CanVendorSystec::vendor_diagnostics() noexcept {

  CanDiagnostics diagnostics{};
  tStatusStruct status;
  // TODO check return code of these functions...
  auto handle = get_module_handle();
  UcanGetStatusEx(handle, m_channel_number, &status);
  WORD can_status = status.m_wCanStatus;
  switch (can_status) {
    case USBCAN_CANERR_OK:        diagnostics.state = "USBCAN_CANERR_OK";         break;
    case USBCAN_CANERR_XMTFULL:   diagnostics.state = "USBCAN_CANERR_XMTFULL";    break;
    case USBCAN_CANERR_OVERRUN:   diagnostics.state = "USBCAN_CANERR_OVERRUN";    break;
    case USBCAN_CANERR_BUSLIGHT:  diagnostics.state = "USBCAN_CANERR_BUSLIGHT";   break;
    case USBCAN_CANERR_BUSHEAVY:  diagnostics.state = "USBCAN_CANERR_BUSHEAVY";   break;
    case USBCAN_CANERR_BUSOFF:    diagnostics.state = "USBCAN_CANERR_BUSOFF";     break;
    case USBCAN_CANERR_QOVERRUN:  diagnostics.state = "USBCAN_CANERR_QOVERRUN";   break;
    case USBCAN_CANERR_QXMTFULL:  diagnostics.state = "USBCAN_CANERR_QXMTFULL";   break;
    case USBCAN_CANERR_REGTEST:   diagnostics.state = "USBCAN_CANERR_REGTEST";    break;
    case USBCAN_CANERR_TXMSGLOST: diagnostics.state = "USBCAN_CANERR_TXMSGLOST";  break;
  }
  tUcanMsgCountInfo msg_count_info;
  UcanGetMsgCountInfoEx(handle, m_channel_number, &msg_count_info);
  diagnostics.tx = msg_count_info.m_wSentMsgCount;
  diagnostics.rx = msg_count_info.m_wRecvdMsgCount;

  DWORD tx_error, rx_error;
  UcanGetCanErrorCounter(handle, m_channel_number, &tx_error, &rx_error);
  diagnostics.tx_error = tx_error;
  diagnostics.rx_error = rx_error;

  tUcanHardwareInfo hw_info;
  if (UcanGetHardwareInfo(handle, &hw_info) != USBCAN_SUCCESSFUL)
    diagnostics.mode = "OFFLINE";
  else switch (hw_info.m_bMode) {
    case kUcanModeNormal:     diagnostics.mode = "NORMAL";      break;
    case kUcanModeListenOnly: diagnostics.mode = "LISTEN_ONLY"; break;
    case kUcanModeTxEcho:     diagnostics.mode = "LOOPBACK";    break;
  }

  DWORD module_time; // in ms
  UcanGetModuleTime(handle, &module_time);
  diagnostics.uptime = (uint32_t) module_time / 1000;

  return diagnostics;
};


/**
 * thread to handle reception of Can messages from the systec device
 */
int CanVendorSystec::SystecRxThread()
{
  BYTE status;
  tCanMsgStruct read_can_message;
  LOG(Log::DBG, CanLogIt::h()) << "SystecRxThread Started. m_receive_thread_flag = [" << m_receive_thread_flag <<"]";
  size_t to_read;
  while (m_receive_thread_flag) {
    to_read = m_queued_reads;
    if (to_read < 1) continue; 
    status = UcanReadCanMsgEx(get_module_handle(), (BYTE *) &m_channel_number, &read_can_message, NULL);
    switch (status) {
      case USBCAN_WARN_SYS_RXOVERRUN:
      case USBCAN_WARN_DLL_RXOVERRUN:
      case USBCAN_WARN_FW_RXOVERRUN:
        LOG(Log::WRN, CanLogIt::h()) << UsbCanGetErrorText(status);
        [[ fallthrough ]];
      case USBCAN_SUCCESSFUL: {
        --m_queued_reads;
        if (read_can_message.m_bFF & USBCAN_MSG_FF_RTR) break;
        std::vector<char> data(read_can_message.m_bData, read_can_message.m_bData + read_can_message.m_bDLC);
        CanFrame can_frame(read_can_message.m_dwID, data, read_can_message.m_bFF);
        received(can_frame);
        break;
      }
      case USBCAN_WARN_NODATA:
        m_queued_reads -= to_read;
        LOG(Log::WRN, CanLogIt::h()) << UsbCanGetErrorText(status);
        break;
        default: // errors
        // USBCAN_ERR_MAXINSTANCES, USBCAN_ERR_ILLHANDLE, USBCAN_ERR_CANNOTINIT,
        // USBCAN_ERR_ILLPARAM, USBCAN_ERR_ILLHW, USBCAN_ERR_ILLCHANNEL
        // TODO should we raise some error state here?
        LOG(Log::ERR, CanLogIt::h()) << UsbCanGetErrorText(status);
        break;
    }
  }

  return 0;
}

std::string CanVendorSystec::UsbCanGetErrorText( long err_code ) {
  switch( err_code ){
  case USBCAN_SUCCESSFUL: return("success");

  case USBCAN_ERR_RESOURCE: return ("This error code returns if one resource could not be generated. In this "
  "case the term resource means memory and handles provided by the	Windows OS");

  case USBCAN_ERR_MAXMODULES: return("An application has tried to open more than 64 USB-CANmodul devices. "
  "The standard version of the DLL only supports up to 64 USB-CANmodul "
  "devices at the same time. This error also appears if several applications "
  "try to access more than 64 USB-CANmodul devices. For example, "
  "application 1 has opened 60 modules, application 2 has opened 4 "
  "modules and application 3 wants to open a module. Application 3 "
  "receives this error code.");

  case USBCAN_ERR_HWINUSE: return("An application tries to initialize an USB-CANmodul with the given device "
  "number. If this module has already been initialized by its own or by "
  "another application, this error code is returned.");

  case USBCAN_ERR_ILLVERSION: return("This error code returns if the firmware version of the USB-CANmodul is "
  "not compatible to the software version of the DLL. In this case, install "
  "the latest driver for the USB-CANmodul. Furthermore make sure that "
  "the latest firmware version is programmed to the USB-CANmodul.");

  case USBCAN_ERR_ILLHW: return("This error code returns if an USB-CANmodul with the given device "
  "number is not found. If the function UcanInitHardware() or "
  "UcanInitHardwareEx() has been called with the device number "
  "USBCAN_ANY_MODULE, and the error code appears, it indicates that "
  "no module is connected to the PC or all connected modules are already "
  "in use.");

  case USBCAN_ERR_ILLHANDLE: return("This error code returns if a function received an incorrect USBCAN "
  "handle. The function first checks which USB-CANmodul is related to this "
  "handle. This error occurs if no device belongs this handle.");

  case USBCAN_ERR_ILLPARAM: return("This error code returns if a wrong parameter is passed to the function. "
  "For example, the value NULL has been passed to a pointer variable "
  "instead of a valid address.");

  case USBCAN_ERR_BUSY: return("This error code occurs if several threads are accessing an "
  "USB-CANmodul within a single application. After the other threads have "
  "finished their tasks, the function may be called again.");

  case USBCAN_ERR_TIMEOUT: return("This error code occurs if the function transmits a command to the "
  "USB-CANmodul but no reply is returned. To solve this problem, close "
  "the application, disconnect the USB-CANmodul, and connect it again.");

  case USBCAN_ERR_IOFAILED: return("This error code occurs if the communication to the kernel driver was "
  "interrupted. This happens, for example, if the USB-CANmodul is "
  "disconnected during transferring data or commands to the "
  "USB-CANmodul.");

  case USBCAN_ERR_DLL_TXFULL: return("The function UcanWriteCanMsg() or UcanWriteCanMsgEx() first checks "
  "if the transmit buffer within the DLL has enough capacity to store new "
  "CAN messages. If the buffer is full, this error code returns. The CAN "
  "message passed to these functions will not be written into the transmit "
  "buffer in order to protect other CAN messages against overwriting. The "
  "size of the transmit buffer is configurable (refer to function "
  "UcanInitCanEx() and structure tUcanInitCanParam).");

  case USBCAN_ERR_MAXINSTANCES: return("A maximum amount of 64 applications are able to have access to the "
  "DLL. If more applications attempting to access to the DLL, this error "
  "code is returned. In this case, it is not possible to use an "
  "USB-CANmodul by this application.");

  case USBCAN_ERR_CANNOTINIT: return("This error code returns if an application tries to call an API function "
  "which only can be called in software state CAN_INIT but the current "
  "software is still in state HW_INIT. Refer to section 4.3.1 and Table 11 for "
  "detailed information.");

  case USBCAN_ERR_DISCONNECT: return("This error code occurs if an API function was called for an "
  "USB-CANmodul that was plugged-off from the computer recently.");

  case USBCAN_ERR_ILLCHANNEL: return("This error code is returned if an extended function of the DLL is called "
  "with parameter bChannel_p = USBCAN_CHANNEL_CH1, but a single-channel USB-CANmodul was used.");

  case USBCAN_ERR_ILLHWTYPE: return("This error code occurs if an extended function of the DLL was called for "
  "a hardware which does not support the feature.");

  case USBCAN_ERRCMD_NOTEQU: return("This error code occurs during communication between the PC and an "
  "USB-CANmodul. The PC sends a command to the USB-CANmodul, "
  "then the module executes the command and returns a response to the "
  "PC. This error code returns if the reply does not correspond to the command.");

  case USBCAN_ERRCMD_REGTST: return("The software tests the CAN controller on the USB-CANmodul when the "
  "CAN interface is initialized. Several registers of the CAN controller are "
  "checked. This error code returns if an error appears during this register test.");

  case USBCAN_ERRCMD_ILLCMD: return("This error code returns if the USB-CANmodul receives a non-defined "
  "command. This error represents a version conflict between the firmware in the USB-CANmodul and the DLL.");

  case USBCAN_ERRCMD_EEPROM: return("The USB-CANmodul has a built-in EEPROM. This EEPROM contains "
  "several configurations, e.g. the device number and the serial number. If "
  "an error occurs while reading these values, this error code is returned.");

  case USBCAN_ERRCMD_ILLBDR: return("The USB-CANmodul has been initialized with an invalid baud rate (refer "
  "to section 4.3.4).");

  case USBCAN_ERRCMD_NOTINIT: return("It was tried to access a CAN-channel of a multi-channel "
  "USB-CANmodul that was not initialized.");

  case USBCAN_ERRCMD_ALREADYINIT: return("The accessed CAN-channel of a multi-channel USB-CANmodul was "
  "already initialized");

  case USBCAN_ERRCMD_ILLSUBCMD: return("An internal error occurred within the DLL. In this case an unknown sub- "
  "command was called instead of a main command (e.g. for the cyclic CAN message-feature).");

  case USBCAN_ERRCMD_ILLIDX: return("An internal error occurred within the DLL. In this case an invalid index "
  "for a list was delivered to the firmware (e.g. for the cyclic CAN message-feature).");

  case USBCAN_ERRCMD_RUNNING: return("The caller tries to define a new list of cyclic CAN messages but this "
  "feature was already started. For defining a new list, it is necessary to stop the feature beforehand.");

  case USBCAN_WARN_NODATA: return("If the function UcanReadCanMsg() or UcanReadCanMsgEx() returns "
  "with this warning, it is an indication that the receive buffer contains no CAN messages.");

  case USBCAN_WARN_SYS_RXOVERRUN: return("This is returned by UcanReadCanMsg() or UcanReadCanMsgEx() if the "
  "receive buffer within the kernel driver runs over. The function "
  "nevertheless returns a valid CAN message. It also indicates that at least "
  "one CAN message are lost. However, it does not indicate the position of the lost CAN messages.");

  case USBCAN_WARN_DLL_RXOVERRUN: return("The DLL automatically requests CAN messages from the "
  "USB-CANmodul and stores the messages into a buffer of the DLL. If "
  "more CAN messages are received than the DLL buffer size allows, this "
  "error code returns and CAN messages are lost. However, it does not "
  "indicate the position of the lost CAN messages. The size of the receive "
  "buffer is configurable (refer to function UcanInitCanEx() and structure "
  "tUcanInitCanParam).");

  case USBCAN_WARN_FW_TXOVERRUN: return("This warning is returned by function UcanWriteCanMsg() or "
  "UcanWriteCanMsgEx() if flag USBCAN_CANERR_QXMTFULL is set in "
  "the CAN driver status. However, the transmit CAN message could be "
  "stored to the DLL transmit buffer. This warning indicates that at least "
  "one transmit CAN message got lost in the device firmware layer. This "
  "warning does not indicate the position of the lost CAN message.");

  case USBCAN_WARN_FW_RXOVERRUN: return("This warning is returned by function UcanWriteCanMsg() or "
  "UcanWriteCanMsgEx() if flag USBCAN_CANERR_QOVERRUN or flag "
  "USBCAN_CANERR_OVERRUN are set in the CAN driver status. The "
  "function has returned with a valid CAN message. This warning indicates "
  "that at least one received CAN message got lost in the firmware layer. "
  "This warning does not indicate the position of the lost CAN message.");

  case USBCAN_WARN_NULL_PTR: return("This warning is returned by functions UcanInitHwConnectControl() or "
  "UcanInitHwConnectControlEx() if a NULL pointer was passed as callback function address.");

  case USBCAN_WARN_TXLIMIT: return("This warning is returned by the function UcanWriteCanMsgEx() if it was "
  "called to transmit more than one CAN message, but a part of them "
  "could not be stored to the transmit buffer within the DLL (because the "
  "buffer is full). The returned variable addressed by the parameter "
  "pdwCount_p indicates the number of CAN messages which are stored "
  "successfully to the transmit buffer.");

  default: return("unknown error code");
  }
}