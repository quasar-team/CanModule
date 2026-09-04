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
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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

/**
 * @brief Reads a field's value out of a configuration as a string
 *
 * @param config The configuration to read from.
 * @param field The field to read.
 * @return The value, or std::nullopt if the field is unset.
 */
std::optional<std::string> value_as_string(const CanDeviceConfiguration& config,
                                           const FieldDescriptor& field) {
  return std::visit(
      [&config](auto member) -> std::optional<std::string> {
        const auto& value = config.*member;
        if (!value.has_value()) return std::nullopt;

        using ValueType = typename std::decay_t<decltype(value)>::value_type;
        if constexpr (std::is_same_v<ValueType, std::string>) {
          return value.value();
        } else if constexpr (std::is_same_v<ValueType, uint32_t>) {
          return std::to_string(value.value());
        } else if constexpr (std::is_same_v<ValueType, bool>) {
          return value.value() ? "true" : "false";
        }

        return std::nullopt;
      },
      field.member);
}

/**
 * @brief Sets a field's value on a configuration, parsing it from a string
 *
 * @param config The configuration to update.
 * @param field The field to set.
 * @param key The name of the parameter, used in errors.
 * @param value The value to parse and assign.
 * @throws std::invalid_argument if the value cannot be converted to the
 * field's type.
 */
void assign_from_string(CanDeviceConfiguration& config,
                        const FieldDescriptor& field, const std::string& key,
                        const std::string& value) {
  std::visit(
      [&](auto member) {
        using ValueType =
            typename std::decay_t<decltype(config.*member)>::value_type;
        if constexpr (std::is_same_v<ValueType, std::string>) {
          config.*member = value;
        } else if constexpr (std::is_same_v<ValueType, uint32_t>) {
          config.*member = to_uint32(key, value);
        } else if constexpr (std::is_same_v<ValueType, bool>) {
          config.*member = to_bool(key, value);
        }
      },
      field.member);
}

}  // namespace

CanDeviceConfiguration CanDeviceConfiguration::from_map(
    const std::map<std::string, std::string>& parameters) {
  CanDeviceConfiguration config;

  for (const auto& parameter : parameters) {
    const std::string& key = parameter.first;
    const std::string& value = parameter.second;

    const auto it = std::find_if(
        // fields is a static const
        fields().begin(), fields().end(),
        [&key](const FieldDescriptor& field) { return field.name == key; });

    if (it == fields().end()) {
      throw std::invalid_argument(
          "Unknown CAN device configuration parameter '" + key + "'");
    }

    assign_from_string(config, *it, key, value);
  }

  return config;
}

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

  for (const auto& field : fields()) {
    const auto value = value_as_string(*this, field);
    if (!value.has_value()) continue;

    if (!first) oss << ", ";
    oss << field.name << "=";
    // Only add quotes if its a string
    if (std::holds_alternative<
            std::optional<std::string> CanDeviceConfiguration::*>(
            field.member)) {
      oss << std::quoted(value.value());
    } else {
      oss << value.value();
    }
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
std::ostream& operator<<(std::ostream& os,
                         const CanDeviceConfiguration& config) noexcept {
  return os << config.to_string();
}

/**
 * @brief Lists the name and value of every parameter that was provided.
 *
 * @return A vector of (name, value) pairs, one for each optional field that
 * holds a value.
 */
std::vector<std::pair<std::string, std::string>>
CanDeviceConfiguration::set_parameters() const noexcept {
  std::vector<std::pair<std::string, std::string>> parameters;

  for (const auto& field : fields()) {
    if (auto value = value_as_string(*this, field)) {
      parameters.emplace_back(field.name, *value);
    }
  }

  return parameters;
}
