#include "CanFrame.h"

#include <stdexcept>

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
    throw std::invalid_argument("Invalid CAN frame: ID must be 29 bits.");
  }

  if (is_29_bits_id() && !is_11_bits_id() &&
      !(m_flags & CanFlags::EXTENDED_ID)) {
    throw std::invalid_argument(
        "Invalid CAN frame: Extended ID flag is not set.");
  }

  if (m_message.size() > 8) {
    throw std::invalid_argument(
        "Invalid CAN frame: Message length exceeds 8 bytes.");
  }

  if (m_requested_length > 0 && !(m_flags & CanFlags::REMOTE_REQUEST)) {
    throw std::invalid_argument(
        "Invalid CAN frame: Requested length is set but not a remote request.");
  }

  if (m_message.size() > 0 && (m_flags & CanFlags::REMOTE_REQUEST)) {
    throw std::invalid_argument(
        "Invalid CAN frame: Message is present but it's a remote request.");
  }

  if (m_requested_length > 8) {
    throw std::invalid_argument(
        "Invalid CAN frame: Requested length exceeds 8 bytes.");
  }

  if (m_message.size() > 0 && m_requested_length > 0) {
    throw std::invalid_argument(
        "Invalid CAN frame: Both message and requested length are present.");
  }
}
