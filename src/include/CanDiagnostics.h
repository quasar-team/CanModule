#ifndef SRC_INCLUDE_CANDIAGNOSTICS_H_
#define SRC_INCLUDE_CANDIAGNOSTICS_H_

#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using optional_string_vector_t = std::optional<std::vector<std::string>>;
using optional_string_t = std::optional<std::string>;
using optional_uint32_vector_t = std::optional<std::vector<uint32_t>>;

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
  std::optional<uint32_t>
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
  std::optional<uint32_t>
      number_connected_clients;  ///< Optional number of connected clients for
                                 ///< Anagate devices.

  std::optional<float>
      temperature;  ///< Optional temperature reading for Anagate devices.
  std::optional<uint32_t> uptime;  ///< Optional uptime for Anagate devices.

  std::optional<uint32_t> tcp_rx;  ///< Optional TCP Received counter for
                                   ///< both SocketCAN and Anagate devices.
  std::optional<uint32_t> tcp_tx;  ///< Optional TCP Transmitted counter for
                                   ///< both SocketCAN and Anagate devices.
  std::optional<uint32_t> rx;      ///< Optional CAN Received counter for
                                   ///< both SocketCAN and Anagate devices.
  std::optional<uint32_t> tx;      ///< Optional CAN Transmitted counter for
                                   ///< both SocketCAN and Anagate devices.
  std::optional<uint32_t>
      rx_error;  ///< Optional CAN Bus Receive Error counter for both
                 ///< SocketCAN and Anagate devices.
  std::optional<uint32_t>
      tx_error;  ///< Optional CAN Bus Transmit Error counter for both
                 ///< SocketCAN and Anagate devices.
  std::optional<uint32_t> rx_drop;     ///< Optional CAN Discarded Rx Full
                                       ///< Queue counter for Anagate devices.
  std::optional<uint32_t> tx_drop;     ///< Optional CAN Discarded Tx Full
                                       ///< Queue counter for Anagate devices.
  std::optional<uint32_t> tx_timeout;  ///< Optional CAN Transmit Timeout
                                       ///< counter for Anagate devices.

  std::optional<uint32_t>
      bus_error;  ///< Optional Number of Bus errors for SocketCAN.
  std::optional<uint32_t> error_warning;  ///< Optional Changes to error warning
                                          ///< state for SocketCAN.
  std::optional<uint32_t> error_passive;  ///< Optional Changes to error passive
                                          ///< state for SocketCAN.
  std::optional<uint32_t>
      bus_off;  ///< Optional Changes to bus off state for SocketCAN.
  std::optional<uint32_t>
      arbitration_lost;  ///< Optional Arbitration lost errors for SocketCAN.
  std::optional<uint32_t>
      restarts;  ///< Optional CAN controller re-starts for SocketCAN.

  std::string to_string() const noexcept;

 private:
  template <typename T>
  static void print_vector_field(std::ostringstream& oss, const T& vector_field,
                                 const std::string& title) {
    if (vector_field.has_value()) {
      oss << title << ": [";
      for (const auto& entry : vector_field.value()) {
        oss << "\"" << entry << "\"" << ", ";
      }
      oss.seekp(-2, std::ios_base::end);  // Remove trailing comma and space
      oss << "]" << std::endl;
    }
  }

  template <typename T>
  static void print_field(std::ostringstream& oss, const T& field,
                          const std::string& title) {
    if (field.has_value()) {
      oss << title << ": " << "\"" << field.value() << "\"" << std::endl;
    }
  }
};

std::ostream& operator<<(std::ostream& os,
                         const CanDiagnostics& config) noexcept;

#endif  // SRC_INCLUDE_CANDIAGNOSTICS_H_
