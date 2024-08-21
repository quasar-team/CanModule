#ifndef SRC_INCLUDE_CANDIAGNOSTICS_H_
#define SRC_INCLUDE_CANDIAGNOSTICS_H_

#include <optional>
#include <string>
#include <vector>

using optional_string_vector_t = std::optional<std::vector<std::string>>;
using optional_string_t = std::optional<std::string>;
using optional_uint32_vector_t = std::optional<std::vector<u_int32_t>>;

/**
 * @brief CanDiagnostic struct to store diagnostic information.
 *
 * This struct contains various fields to store diagnostic information related
 * to CAN communication. The fields are wrapped in std::optional to indicate
 * that they may not always be available.
 */
struct CanDiagnostics {
  optional_string_vector_t log_entries;  ///< Optional vector of log entries.

  optional_string_t name;     ///< Optional SocketCAN device name.
  std::optional<int> handle;  ///< Optional handle for Anagate devices.
  optional_string_t
      mode;  ///< Optional mode for both SocketCAN and Anagate devices.
  optional_string_t
      state;  ///< Optional state for both SocketCAN and Anagate devices.
  std::optional<u_int32_t>
      bitrate;  ///< Optional bitrate for both SocketCAN and Anagate devices.

  optional_string_vector_t
      connected_clients_addresses;  ///< Optional vector of connected clients'
                                    ///< addresses for Anagate devices.
  optional_string_vector_t
      connected_clients_timestamps;  ///< Optional vector of connected clients'
                                     ///< timestamps for Anagate devices.
  optional_uint32_vector_t
      connected_clients_ports;  ///< Optional vector of connected clients' ports
                                ///< for Anagate devices.
  std::optional<u_int32_t>
      number_connected_clients;  ///< Optional number of connected clients for
                                 ///< Anagate devices.

  std::optional<float>
      temperature;  ///< Optional temperature reading for Anagate devices.
  std::optional<u_int32_t> uptime;  ///< Optional uptime for Anagate devices.

  std::optional<u_int32_t> countTCPRx;  ///< Optional TCP Received counter for
                                        ///< both SocketCAN and Anagate devices.
  std::optional<u_int32_t>
      countTCPTx;  ///< Optional TCP Transmitted counter for both SocketCAN and
                   ///< Anagate devices.
  std::optional<u_int32_t> countCANRx;  ///< Optional CAN Received counter for
                                        ///< both SocketCAN and Anagate devices.
  std::optional<u_int32_t>
      countCANTx;  ///< Optional CAN Transmitted counter for both SocketCAN and
                   ///< Anagate devices.
  std::optional<u_int32_t>
      countCANRxErr;  ///< Optional CAN Bus Receive Error counter for both
                      ///< SocketCAN and Anagate devices.
  std::optional<u_int32_t>
      countCANTxErr;  ///< Optional CAN Bus Transmit Error counter for both
                      ///< SocketCAN and Anagate devices.
  std::optional<u_int32_t>
      countCANRxDisc;  ///< Optional CAN Discarded Rx Full Queue counter for
                       ///< Anagate devices.
  std::optional<u_int32_t>
      countCANTxDisc;  ///< Optional CAN Discarded Tx Full Queue counter for
                       ///< Anagate devices.
  std::optional<u_int32_t> countCANTimeout;  ///< Optional CAN Transmit Timeout
                                             ///< counter for Anagate devices.
};

#endif  // SRC_INCLUDE_CANDIAGNOSTICS_H_
