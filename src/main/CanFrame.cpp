#include "CanFrame.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "CanLogIt.h"

/**
 * @brief Validates the CAN frame based on various conditions.
 *
 * This function checks multiple conditions to determine if the CAN frame is
 * valid. If any condition is not met, it throws a `std::invalid_argument`
 * exception with an appropriate error message.
 *
 * @return void This function does not return a value.
 * @throws std::invalid_argument If the CAN frame is not valid.
 */
void CanFrame::validate_frame() {
  if (!is_29_bits_id()) {
    LOG(Log::ERR, CanLogIt::h()) << "Invalid CAN frame: ID must be 29 bits.";
    throw std::invalid_argument("Invalid CAN frame: ID must be 29 bits.");
  }

  if (is_29_bits_id() && !is_11_bits_id() &&
      !(m_flags & can_flags::extended_id)) {
    LOG(Log::ERR, CanLogIt::h())
        << "Invalid CAN frame: Extended ID flag is not set.";
    throw std::invalid_argument(
        "Invalid CAN frame: Extended ID flag is not set.");
  }

  if (m_message.size() > 8) {
    LOG(Log::ERR, CanLogIt::h())
        << "Invalid CAN frame: Message length exceeds 8 bytes.";
    throw std::invalid_argument(
        "Invalid CAN frame: Message length exceeds 8 bytes.");
  }

  if (m_requested_length > 0 && !(m_flags & can_flags::remote_request)) {
    LOG(Log::ERR, CanLogIt::h())
        << "Invalid CAN frame: Requested length is set "
           "but not a remote request.";
    throw std::invalid_argument(
        "Invalid CAN frame: Requested length is set but not a remote request.");
  }

  if (m_message.size() > 0 && (m_flags & can_flags::remote_request)) {
    LOG(Log::ERR, CanLogIt::h())
        << "Invalid CAN frame: Message is present but it's a remote request.";
    throw std::invalid_argument(
        "Invalid CAN frame: Message is present but it's a remote request.");
  }

  if (m_requested_length > 8) {
    LOG(Log::ERR, CanLogIt::h())
        << "Invalid CAN frame: Requested length exceeds 8 bytes.";
    throw std::invalid_argument(
        "Invalid CAN frame: Requested length exceeds 8 bytes.");
  }

  if (m_message.size() > 0 && m_requested_length > 0) {
    LOG(Log::ERR, CanLogIt::h())
        << "Invalid CAN frame: Both message and requested length are present.";
    throw std::invalid_argument(
        "Invalid CAN frame: Both message and requested length are present.");
  }

  LOG(Log::TRC, CanLogIt::h()) << "CAN frame is valid: " << to_string();
}

/**
 * @brief Converts the CAN frame to a string representation.
 *
 * This function generates a string representation of the CAN frame, including
 * the ID, length, message bytes, and any flags (extended ID, remote request,
 * error). The string format is as follows:
 *
 * "ID LEN BYTE1 BYTE2 ... BYTEN [EXT] [RTR] [ERR]"
 *
 * - ID: 3-digit hexadecimal representation of the CAN frame ID.
 * - LEN: Number of message bytes.
 * - BYTE1, BYTE2, ..., BYTEN: Message bytes in hexadecimal format.
 * - EXT: Indicates that the frame has an extended ID.
 * - RTR: Indicates that the frame is a remote request.
 * - ERR: Indicates that the frame has an error.
 *
 * @return std::string The string representation of the CAN frame.
 */
std::string CanFrame::to_string() const noexcept {
  std::ostringstream oss;

  int id_padding{3};
  if (is_extended_id()) {
    id_padding = 7;
  }

  oss << std::setfill('0') << std::setw(id_padding) << std::hex << id();

  oss << " " << length();

  for (const auto& byte : message()) {
    oss << " " << std::setfill('0') << std::setw(2) << std::hex
        << static_cast<int>(byte);
  }

  if (is_extended_id()) {
    oss << " EXT";
  }

  if (is_remote_request()) {
    oss << " RTR";
  }

  if (is_error()) {
    oss << " ERR";
  }

  return oss.str();
}

/**
 * @brief Overloads the << operator to insert the string representation of a CAN
 * frame into an output stream.
 *
 * This function calls the `to_string()` method of the `CanFrame` object and
 * inserts the resulting string into the provided output stream.
 *
 * @param os The output stream to which the CAN frame's string representation
 * will be inserted.
 * @param frame The CAN frame object whose string representation will be
 * inserted into the output stream.
 *
 * @return std::ostream& The output stream (os) with the CAN frame's string
 * representation inserted.
 */
std::ostream& operator<<(std::ostream& os, const CanFrame& frame) noexcept {
  return os << frame.to_string();
}
