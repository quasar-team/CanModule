#include <cstdint>
#include <vector>

namespace CanFlags {
const uint32_t STANDARD_ID = 0;
const uint32_t EXTENDED_ID = 1;
const uint32_t ERROR_FRAME = 1 << 1;
const uint32_t REMOTE_FRAME = 1 << 2;
};  // namespace CanFlags

struct CanFrame {
  CanFrame(const uint32_t id, const uint32_t requested_length,
           const uint32_t flags);
  CanFrame(const uint32_t id, const uint32_t requested_length)
      : CanFrame(id, requested_length,
                 CanFlags::REMOTE_FRAME | is_id_standard(id)
                     ? CanFlags::STANDARD_ID
                     : CanFlags::EXTENDED_ID) {};
  CanFrame(const uint32_t id, const std::vector<char>& message)
      : CanFrame(id, message,
                 is_id_standard(id) ? CanFlags::STANDARD_ID
                                    : CanFlags::EXTENDED_ID) {};
  CanFrame(const uint32_t id, const std::vector<char>& message,
           const uint32_t flags);

 private:
  const uint32_t m_id{0};
  const uint32_t m_requested_length{0};
  const std::vector<char> m_message;
  const uint32_t m_flags{};

  bool m_valid{false};

  static bool is_id_standard(uint32_t id);
  static bool is_id_extended(uint32_t id);
};
