#include "CanDeviceConfiguration.h"

#include <iomanip>
#include <iostream>
#include <sstream>

/**
 * @brief Converts the CanDeviceConfiguration object to a string representation.
 *
 * This function generates a string representation of the CanDeviceConfiguration
 * object, including the values of its member variables. The string is formatted
 * as a comma-separated list of key-value pairs, where the keys correspond to
 * the member variable names and the values represent their respective values.
 *
 * @return A string representation of the CanDeviceConfiguration object.
 */
std::string CanDeviceConfiguration::to_string() const {
  std::ostringstream oss;
  bool first = true;

  if (bus_name.has_value()) {
    if (!first) oss << ", ";
    oss << "bus_name=" << std::quoted(bus_name.value());
    first = false;
  }

  if (bus_address.has_value()) {
    if (!first) oss << ", ";
    oss << "bus_address=" << bus_address.value();
    first = false;
  }

  if (host.has_value()) {
    if (!first) oss << ", ";
    oss << "host=" << std::quoted(host.value());
    first = false;
  }

  if (bitrate.has_value()) {
    if (!first) oss << ", ";
    oss << "bitrate=" << bitrate.value();
    first = false;
  }

  if (enable_termination.has_value()) {
    if (!first) oss << ", ";
    oss << "enable_termination="
        << (enable_termination.value() ? "true" : "false");
    first = false;
  }

  if (timeout.has_value()) {
    if (!first) oss << ", ";
    oss << "timeout=" << timeout.value();
    first = false;
  }

  if (sent_acknowledgement.has_value()) {
    if (!first) oss << ", ";
    oss << "sent_acknowledgement=" << sent_acknowledgement.value();
    first = false;
  }

  return oss.str();
}

/**
 * @brief Overloads the << operator to print a CanDeviceConfiguration object to
 * an output stream.
 *
 * This function calls the to_string() method of the CanDeviceConfiguration
 * object and inserts the resulting string into the provided output stream. This
 * allows for convenient printing of CanDeviceConfiguration objects using the
 * standard << operator.
 *
 * @param os The output stream to which the CanDeviceConfiguration object will
 * be printed.
 * @param config The CanDeviceConfiguration object to be printed.
 *
 * @return The output stream (os) with the CanDeviceConfiguration object's
 * string representation inserted.
 */
std::ostream &operator<<(std::ostream &os,
                         const CanDeviceConfiguration &config) {
  return os << config.to_string();
}
