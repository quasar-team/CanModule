#ifndef SRC_INCLUDE_CANFRAME_H_
#define SRC_INCLUDE_CANFRAME_H_
#include <cstdint>
#include <vector>

namespace CanFlags {
constexpr int STANDARD_ID = 0 << 0;
constexpr int EXTENDED_ID = 1 << 0;
constexpr int ERROR_FRAME = 1 << 1;
constexpr int REMOTE_REQUEST = 1 << 2;
};  // namespace CanFlags

struct CanFrame {
  CanFrame(const uint32_t id, const uint32_t requested_length,
           const uint32_t flags)
      : m_id(id), m_requested_length(requested_length), m_flags(flags) {
    if (!is_remote_request()) {
      return;
    }
    validate_frame();
  }
  CanFrame(const uint32_t id, const uint32_t requested_length)
      : CanFrame(id, requested_length,
                 CanFlags::REMOTE_REQUEST |
                     (is_11_bits(id) ? CanFlags::STANDARD_ID
                                     : CanFlags::EXTENDED_ID)) {}
  explicit CanFrame(const uint32_t id) : CanFrame(id, 0) {}
  CanFrame(const uint32_t id, const std::vector<char>& message)
      : CanFrame(
            id, message,
            is_11_bits(id) ? CanFlags::STANDARD_ID : CanFlags::EXTENDED_ID) {}
  CanFrame(const uint32_t id, const std::vector<char>& message,
           const uint32_t flags)
      : m_id(id), m_message(message), m_flags(flags) {
    validate_frame();
  }

  bool is_valid() const { return m_valid; }
  uint32_t id() const { return m_id; }
  std::vector<char> message() const { return m_message; }
  uint32_t flags() const { return m_flags; }
  bool is_error_frame() const { return (m_flags & CanFlags::ERROR_FRAME) != 0; }
  bool is_remote_request() const {
    return (m_flags & CanFlags::REMOTE_REQUEST) != 0;
  }
  bool is_standard_id() const { return !is_extended_id(); }
  bool is_extended_id() const { return (m_flags & CanFlags::EXTENDED_ID) != 0; }
  uint32_t length() const {
    return (m_flags & CanFlags::REMOTE_REQUEST) ? m_requested_length
                                                : m_message.size();
  }

 private:
  const uint32_t m_id{0};
  const uint32_t m_requested_length{0};
  const std::vector<char> m_message;
  const uint32_t m_flags{};
  bool m_valid{false};

  bool is_11_bits_id() const { return is_11_bits(m_id); }
  bool is_29_bits_id() const { return is_29_bits(m_id); }

  void validate_frame();

  static bool is_11_bits(uint32_t id) { return (id & 0xFFFFF800) == 0; }
  static bool is_29_bits(uint32_t id) { return (id & 0xE0000000) == 0; }
};

#endif  // SRC_INCLUDE_CANFRAME_H_
