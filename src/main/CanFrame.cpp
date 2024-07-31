#include "CanFrame.h"

void CanFrame::validate_frame() {
  if (!is_29_bits_id()) {
    return;
  }

  if (is_29_bits_id() && !is_11_bits_id() &&
      !(m_flags & CanFlags::EXTENDED_ID)) {
    return;
  }

  if (m_message.size() > 8) {
    return;
  }

  if (m_requested_length > 0 && !(m_flags & CanFlags::REMOTE_REQUEST)) {
    return;
  }

  if (m_message.size() > 0 && (m_flags & CanFlags::REMOTE_REQUEST)) {
    return;
  }

  if (m_requested_length > 8) {
    return;
  }

  if (m_message.size() > 0 && m_requested_length > 0) {
    return;
  }

  m_valid = true;
}
