#include "CanDeviceConfiguration.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

/**
 * @brief Converts a string to an unsigned 32 bits integer.
 *
 * @param key The name of the parameter being converted, used in the error.
 * @param value The value to convert, a decimal number without sign.
 * @return The converted value.
 * @throws std::invalid_argument if the value is not an unsigned 32 bits
 * integer.
 */
uint32_t to_uint32(const std::string& key, const std::string& value) {
  const bool is_decimal =
      !value.empty() &&
      std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isdigit(character) != 0;
      });

  if (is_decimal) {
    try {
      const uint64_t parsed = std::stoull(value);

      if (parsed <= std::numeric_limits<uint32_t>::max()) {
        return static_cast<uint32_t>(parsed);
      }
    } catch (const std::out_of_range&) {
      // Reported below, together with the values that are not decimal.
    }
  }

  throw std::invalid_argument("Invalid value '" + value +
                              "' for CAN device "
                              "configuration parameter '" +
                              key + "': expected an unsigned 32 bits integer");
}

/**
 * @brief Converts a string to a boolean.
 *
 * @param key The name of the parameter being converted, used in the error.
 * @param value The value to convert, either "true" or "false".
 * @return The converted value.
 * @throws std::invalid_argument if the value is not a boolean.
 */
bool to_bool(const std::string& key, const std::string& value) {
  if (value == "true") {
    return true;
  }

  if (value == "false") {
    return false;
  }

  throw std::invalid_argument("Invalid value '" + value +
                              "' for CAN device "
                              "configuration parameter '" +
                              key + "': expected 'true' or 'false'");
}

}  // namespace

/**
 * @brief Constructs a configuration from a map of string parameters.
 *
 * @param parameters The configuration parameters, indexed by name.
 * @throws std::invalid_argument if a key is not a configuration parameter or if
 * a value cannot be converted to the type of its parameter.
 */
CanDeviceConfiguration::CanDeviceConfiguration(
    const std::map<std::string, std::string>& parameters) {
  for (const auto& [key, value] : parameters) {
    if (key == "bus_name") {
      bus_name = value;
    } else if (key == "bus_number") {
      bus_number = to_uint32(key, value);
    } else if (key == "host") {
      host = value;
    } else if (key == "bitrate") {
      bitrate = to_uint32(key, value);
    } else if (key == "enable_termination") {
      enable_termination = to_bool(key, value);
    } else if (key == "high_speed") {
      high_speed = to_bool(key, value);
    } else if (key == "timeout") {
      timeout = to_uint32(key, value);
    } else if (key == "vcan") {
      vcan = to_bool(key, value);
    } else if (key == "sent_acknowledgement") {
      sent_acknowledgement = to_uint32(key, value);
    } else {
      throw std::invalid_argument(
          "Unknown CAN device configuration parameter '" + key + "'");
    }
  }
}

CanDeviceConfiguration::CanDeviceConfiguration(
    std::optional<std::string> bus_name, std::optional<uint32_t> bus_number,
    std::optional<std::string> host, std::optional<uint32_t> bitrate,
    std::optional<bool> enable_termination, std::optional<bool> high_speed,
    std::optional<uint32_t> timeout, std::optional<bool> vcan,
    std::optional<uint32_t> sent_acknowledgement)
    : bus_name(std::move(bus_name)),
      bus_number(bus_number),
      host(std::move(host)),
      bitrate(bitrate),
      enable_termination(enable_termination),
      high_speed(high_speed),
      timeout(timeout),
      vcan(vcan),
      sent_acknowledgement(sent_acknowledgement) {}

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
std::string CanDeviceConfiguration::to_string() const noexcept {
  std::ostringstream oss;
  bool first = true;

  if (bus_name.has_value()) {
    if (!first) oss << ", ";
    oss << "bus_name=" << std::quoted(bus_name.value());
    first = false;
  }

  if (bus_number.has_value()) {
    if (!first) oss << ", ";
    oss << "bus_number=" << bus_number.value();
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

  if (vcan.has_value()) {
    if (!first) oss << ", ";
    oss << "vcan=" << (vcan.value() ? "true" : "false");
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
    first = false;  // NOLINT: Indeed, this is the last field, but we set first
                    // to false for consistency.
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
std::ostream& operator<<(std::ostream& os,
                         const CanDeviceConfiguration& config) noexcept {
  return os << config.to_string();
}
