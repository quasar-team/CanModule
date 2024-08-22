#include "CanVendorAnagate.h"

#include <AnaGateDllCan.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>  // NOLINT

std::mutex CanVendorAnagate::m_handles_lock;
std::map<int, CanVendorAnagate *> CanVendorAnagate::m_handles;

/**
 * @brief Callback function to handle incoming CAN frames from the AnaGate DLL.
 *
 * This function is called by the AnaGate DLL whenever a CAN frame is received.
 * It extracts the necessary information from the parameters and constructs a
 * CanFrame object, which is then passed to the appropriate CanVendorAnagate
 * instance.
 *
 * @param nIdentifier The identifier of the received CAN frame.
 * @param pcBuffer A pointer to the data buffer containing the received CAN
 * frame.
 * @param nBufferLen The length of the received CAN frame data.
 * @param nFlags The flags associated with the received CAN frame.
 * @param hHandle The handle of the CAN device associated with the received CAN
 * frame.
 */
void anagate_receive(AnaInt32 nIdentifier, const char *pcBuffer,
                     AnaInt32 nBufferLen, AnaInt32 nFlags, AnaInt32 hHandle) {
  uint32_t flags{0};
  (nFlags & AnagateConstants::EXTENDED_ID) ? flags |= CanFlags::EXTENDED_ID : 0;
  (nFlags & AnagateConstants::REMOTE_REQUEST)
      ? flags |= CanFlags::REMOTE_REQUEST
      : 0;
  auto o = CanVendorAnagate::m_handles[hHandle];
  if (o != nullptr) {
    if (flags & CanFlags::REMOTE_REQUEST) {
      o->received(CanFrame{static_cast<uint32_t>(nIdentifier),
                           static_cast<uint32_t>(nBufferLen), flags});
    } else {
      o->received(CanFrame{static_cast<uint32_t>(nIdentifier),
                           std::vector<char>(pcBuffer, pcBuffer + nBufferLen),
                           flags});
    }
  }
}

/**
 * @brief Opens a connection to the CAN device associated with the current
 * instance.
 *
 * This function opens a connection to the CAN device associated with the
 * current instance using the provided configuration parameters. It calls the
 * CANOpenDevice function from the AnaGate DLL to establish the connection and
 * sets a callback function to handle incoming CAN frames. The opened device
 * handle is stored in the m_handle member variable.
 *
 * @return An integer representing the status of the operation. A value of 0
 * indicates success, while a non-zero value indicates an error. The specific
 * meaning of the error code can be obtained by calling CANGetLastError.
 */
int CanVendorAnagate::vendor_open() {
  const AnaInt32 result = CANOpenDevice(
      &m_handle, args().config.sent_acknowledgement.value_or(false), true,
      args().config.bus_address.value(), args().config.host.value().c_str(),
      args().config.timeout.value_or(AnagateConstants::default_timeout));
  CANSetCallback(m_handle, reinterpret_cast<CAN_PF_CALLBACK>(anagate_receive));
  std::lock_guard<std::mutex> guard(CanVendorAnagate::m_handles_lock);
  CanVendorAnagate::m_handles[m_handle] = this;
  return static_cast<int>(result);
}

/**
 * @brief Sends a CAN frame to the associated device.
 *
 * This function sends a CAN frame to the CAN device associated with the current
 * instance. It prepares the necessary flags based on the properties of the CAN
 * frame and then calls the appropriate CANWrite function from the AnaGate DLL.
 *
 * @param frame A const reference to the CanFrame object to be sent.
 *
 * @return An integer representing the status of the operation. A value of 0
 * indicates success, while a non-zero value indicates an error. The specific
 * meaning of the error code can be obtained by calling CANGetLastError.
 */
int CanVendorAnagate::vendor_send(const CanFrame &frame) {
  int anagate_flags{0};
  anagate_flags |= frame.is_extended_id() ? AnagateConstants::EXTENDED_ID : 0;
  anagate_flags |=
      frame.is_remote_request() ? AnagateConstants::REMOTE_REQUEST : 0;

  const int sent_status =
      frame.is_remote_request()
          ? CANWrite(m_handle, frame.id(), AnagateConstants::empty_message,
                     frame.length(), anagate_flags)
          : CANWrite(m_handle, frame.id(), frame.message().data(),
                     frame.length(), anagate_flags);
  return sent_status;
}

/**
 * @brief Retrieves diagnostic information from the CAN device associated with
 * the current instance.
 *
 * This function retrieves diagnostic information from the CAN device associated
 * with the current instance. The diagnostic information is returned as a
 * CanDiagnostics object, which contains various parameters such as error
 * counters, bus status, and other relevant diagnostic data.
 *
 * @return A CanDiagnostics object containing the diagnostic information.
 *
 * @note The specific implementation of this function may vary depending on the
 * capabilities of the associated Anagate.
 */
CanDiagnostics CanVendorAnagate::vendor_diagnostics() {
  auto anagate_convert_timestamp = [](AnaInt64 timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm *local_time = std::localtime(&time);
    std::ostringstream timestamp_stream;
    timestamp_stream << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");
    return timestamp_stream.str();
  };

  auto anagate_convert_ip = [](AnaUInt32 ip) {
    std::ostringstream ip_stream;
    ip_stream << ((ip >> 24) & 0xFF) << "." << ((ip >> 16) & 0xFF) << "."
              << ((ip >> 8) & 0xFF) << "." << (ip & 0xFF);
    return ip_stream.str();
  };

  CanDiagnostics diagnostics{};

  constexpr AnaUInt32 nLogID{0};
  AnaUInt32 pnCurrentID{0};
  AnaUInt32 total_entries{0};
  AnaUInt32 pnLogCount{0};
  AnaInt64 pnLogDate{0};
  constexpr unsigned int kBufferSize = 110;
  char pcBuffer[kBufferSize];

  if (CANGetLog(m_handle, nLogID, &pnCurrentID, &total_entries, &pnLogDate,
                pcBuffer) == 0) {
    diagnostics.log_entries.emplace(total_entries);
    for (AnaUInt32 i{0}; i < total_entries; ++i) {
      if (CANGetLog(m_handle, i, &pnCurrentID, &pnLogCount, &pnLogDate,
                    pcBuffer) == 0) {
        std::ostringstream log_entry_stream;
        log_entry_stream << anagate_convert_timestamp(pnLogDate) << " - "
                         << pcBuffer;
        std::string log_entry = log_entry_stream.str();

        (*diagnostics.log_entries)[i] = log_entry;
      }
    }
  }

  AnaInt32 temp{0}, uptime{0};
  if (CANGetDiagData(m_handle, &temp, &uptime) == 0) {
    diagnostics.temperature = temp / 10.0f;
    diagnostics.uptime = uptime;
  }

  constexpr unsigned int kMaxClients = 6;
  AnaUInt32 client_count{0};
  AnaUInt32 ip_addresses[kMaxClients];
  AnaUInt32 ports[kMaxClients];
  AnaInt64 connect_dates[kMaxClients];

  if (CANGetClientList(m_handle, &client_count, ip_addresses, ports,
                       connect_dates) == 0) {
    diagnostics.connected_clients_addresses.emplace(client_count);
    diagnostics.connected_clients_ports.emplace(client_count);
    diagnostics.connected_clients_timestamps.emplace(client_count);
    diagnostics.number_connected_clients = client_count;

    for (AnaUInt32 i{0}; i < client_count; ++i) {
      (*diagnostics.connected_clients_addresses)[i] =
          anagate_convert_ip(ip_addresses[i]);
      (*diagnostics.connected_clients_ports)[i] = ports[i];
      (*diagnostics.connected_clients_timestamps)[i] =
          anagate_convert_timestamp(connect_dates[i]);
    }
  }

  AnaUInt32 pnCountTCPRx{0}, pnCountTCPTx{0}, pnCountCANRx{0}, pnCountCANTx{0},
      pnCountCANRxErr{0}, pnCountCANTxErr{0}, pnCountCANRxDisc{0},
      pnCountCANTxDisc{0}, pnCountCANTimeout{0};
  if (CANGetCounters(m_handle, &pnCountTCPRx, &pnCountTCPTx, &pnCountCANRx,
                     &pnCountCANTx, &pnCountCANRxErr, &pnCountCANTxErr,
                     &pnCountCANRxDisc, &pnCountCANTxDisc,
                     &pnCountCANTimeout) == 0) {
    diagnostics.tcp_rx = pnCountTCPRx;
    diagnostics.tcp_tx = pnCountTCPTx;
    diagnostics.rx = pnCountCANRx;
    diagnostics.tx = pnCountCANTx;
    diagnostics.rx_error = pnCountCANRxErr;
    diagnostics.tx_error = pnCountCANTxErr;
    diagnostics.rx_drop = pnCountCANRxDisc;
    diagnostics.tx_drop = pnCountCANTxDisc;
    diagnostics.tx_timeout = pnCountCANTimeout;
  }

  AnaUInt32 bitrate{0};
  AnaUInt8 operating_mode{0};
  AnaInt32 pbTermination{0}, pbHighSpeed{0}, pbTimestampOn{0};
  if (CANGetGlobals(m_handle, &bitrate, &operating_mode, &pbTermination,
                    &pbHighSpeed, &pbTimestampOn) == 0) {
    diagnostics.bitrate = bitrate;
    switch (operating_mode) {
      case 0:
        diagnostics.mode = "NORMAL";
        break;
      case 1:
        diagnostics.mode = "LOOPBACK";
        break;
      case 2:
        diagnostics.mode = "LISTEN ONLY";
        break;
      case 3:
        diagnostics.mode = "OFFILINE";
        break;
      default:
        break;
    }
  }

  diagnostics.handle = m_handle;

  return diagnostics;
}

/**
 * @brief Closes the CAN device associated with the current instance.
 *
 * This function closes the CAN device associated with the current instance by
 * calling the CANCloseDevice function from the AnaGate DLL. It also removes the
 * instance from the m_handles map to prevent any further use of the closed
 * device.
 *
 * @return An integer representing the status of the operation. A value of 0
 * indicates success, while a non-zero value indicates an error. The specific
 * meaning of the error code can be obtained by calling CANGetLastError.
 */
int CanVendorAnagate::vendor_close() {
  const int r = CANCloseDevice(m_handle);
  std::lock_guard<std::mutex> guard(CanVendorAnagate::m_handles_lock);
  CanVendorAnagate::m_handles.erase(m_handle);
  return r;
}
