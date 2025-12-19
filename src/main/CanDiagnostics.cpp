#include "CanDiagnostics.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

/**
 * @brief Converts the diagnostic information into a human-readable string
 * format.
 *
 * This function collects various diagnostic information from the CANDiagnostics
 * object and formats it into a string. The information includes log entries,
 * name, handle, mode, state, bitrate, connected clients, temperature, uptime,
 * TCP and CAN statistics, and CAN controller errors.
 *
 * @return A string containing the formatted diagnostic information. If no
 * diagnostic information is available, it returns a message indicating that no
 * information is available.
 */
std::string CanDiagnostics::to_string() const noexcept {
  std::ostringstream oss;

  print_vector_field<optional_string_vector_t>(oss, log_entries, "Log Entries");

  print_field<optional_string_t>(oss, name, "Name");
  print_field<std::optional<int>>(oss, handle, "Handle");
  print_field<optional_string_t>(oss, mode, "Mode");
  print_field<optional_string_t>(oss, state, "State");
  print_field<std::optional<uint32_t>>(oss, bitrate, "Bitrate");
  print_vector_field<optional_string_vector_t>(oss, connected_clients_addresses,
                                               "Connected Clients");
  print_vector_field<optional_string_vector_t>(
      oss, connected_clients_timestamps, "Connected Clients' Timestamps");
  print_vector_field<optional_uint32_vector_t>(oss, connected_clients_ports,
                                               "Connected clients");

  print_field<std::optional<float>>(oss, temperature, "Temperature");
  print_field<std::optional<uint32_t>>(oss, uptime, "Uptime");
  print_field<std::optional<uint32_t>>(oss, tcp_rx, "TCP Received");
  print_field<std::optional<uint32_t>>(oss, tcp_tx, "TCP Transmitted");
  print_field<std::optional<uint32_t>>(oss, rx, "CAN Received");
  print_field<std::optional<uint32_t>>(oss, tx, "CAN Transmitted");
  print_field<std::optional<uint32_t>>(oss, rx_error, "CAN Receive Error");
  print_field<std::optional<uint32_t>>(oss, tx_error, "CAN Transmit Error");
  print_field<std::optional<uint32_t>>(oss, rx_drop,
                                       "CAN Discarded Rx Full Queue");
  print_field<std::optional<uint32_t>>(oss, tx_drop,
                                       "CAN Discarded Tx Full Queue");
  print_field<std::optional<uint32_t>>(oss, tx_timeout, "CAN Transmit Timeout");
  print_field<std::optional<uint32_t>>(oss, bus_error, "Number of Bus Errors");
  print_field<std::optional<uint32_t>>(oss, error_warning,
                                       "Changes to Error Warning State");
  print_field<std::optional<uint32_t>>(oss, error_passive,
                                       "Changes to Error Passive State");
  print_field<std::optional<uint32_t>>(oss, bus_off,
                                       "Changes to Bus Off State");
  print_field<std::optional<uint32_t>>(oss, arbitration_lost,
                                       "Arbitration Lost Errors");
  print_field<std::optional<uint32_t>>(oss, restarts,
                                       "CAN Controller Restarts");
  print_field<std::optional<float>>(oss, rx_per_second,
                                    "CAN Receive Average Rate Per Second");
  print_field<std::optional<float>>(oss, tx_per_second,
                                    "CAN Transmit Average Rate Per Second");

  if (oss.str().empty()) {
    oss << "No diagnostic information available." << std::endl;
  }

  return oss.str();
}

/**
 * @brief Overloads the << operator to print the diagnostic information of a
 * CanDiagnostics object.
 *
 * This function calls the to_string() method of the CanDiagnostics object and
 * outputs the resulting string to the provided output stream. This allows for
 * convenient and readable printing of diagnostic information.
 *
 * @param os The output stream to which the diagnostic information will be
 * printed.
 * @param diag The CanDiagnostics object whose diagnostic information will be
 * printed.
 *
 * @return The output stream (os) after the diagnostic information has been
 * printed.
 */
std::ostream& operator<<(std::ostream& os,
                         const CanDiagnostics& diag) noexcept {
  return os << diag.to_string();
}
