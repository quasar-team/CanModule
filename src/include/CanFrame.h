#ifndef SRC_INCLUDE_CANFRAME_H_
#define SRC_INCLUDE_CANFRAME_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @namespace can_flags
 * @brief Namespace containing flag constants for CAN frames.
 */
namespace can_flags {
constexpr int standard_id = 0 << 0;     ///< Standard 11-bit identifier
constexpr int extended_id = 1 << 0;     ///< Extended 29-bit identifier
constexpr int error_frame = 1 << 1;     ///< Error frame flag
constexpr int remote_request = 1 << 2;  ///< Remote request flag
};  // namespace can_flags

/**
 * @struct CanFrame
 * @brief Represents a CAN frame with various attributes and methods.
 */
struct CanFrame {
  /**
   * @brief Constructs a CanFrame with specified ID, requested length, and
   * flags.
   * @param id The identifier of the CAN frame.
   * @param requested_length The requested length of the CAN frame.
   * @param flags The flags associated with the CAN frame.
   * @throws std::invalid_argument if the CAN Frame has unvalid data.
   */
  inline CanFrame(const uint32_t id, const uint32_t requested_length,
                  const uint32_t flags)
      : m_id(id), m_requested_length(requested_length), m_flags(flags) {
    if (!is_remote_request()) {
      throw std::invalid_argument("Invalid CAN frame: Remote Flag is not set.");
    }
    validate_frame();
  }

  /**
   * @brief Constructs a CanFrame with specified ID and requested length.
   * @param id The identifier of the CAN frame.
   * @param requested_length The requested length of the CAN frame.
   * @throws std::invalid_argument if the CAN Frame has unvalid data.
   */
  inline CanFrame(const uint32_t id, const uint32_t requested_length)
      : CanFrame(id, requested_length,
                 can_flags::remote_request |
                     (is_11_bits(id) ? can_flags::standard_id
                                     : can_flags::extended_id)) {}
  /**
   * @brief Constructs a CanFrame with specified ID.
   * @param id The identifier of the CAN frame.
   * @throws std::invalid_argument if the CAN Frame has unvalid data.
   */
  explicit CanFrame(const uint32_t id) : CanFrame(id, std::vector<char>()) {}
  /**
   * @brief Constructs a CanFrame with specified ID and message.
   * @param id The identifier of the CAN frame.
   * @param message The message content of the CAN frame.
   * @throws std::invalid_argument if the CAN Frame has unvalid data.
   */
  inline CanFrame(const uint32_t id, const std::vector<char>& message)
      : CanFrame(
            id, message,
            is_11_bits(id) ? can_flags::standard_id : can_flags::extended_id) {}
  /**
   * @brief Constructs a CanFrame with specified ID, message, and flags.
   * @param id The identifier of the CAN frame.
   * @param message The message content of the CAN frame.
   * @param flags The flags associated with the CAN frame.
   * @throws std::invalid_argument if the CAN Frame has unvalid data.
   */
  inline CanFrame(const uint32_t id, const std::vector<char>& message,
                  const uint32_t flags)
      : CanFrame(id, message.data(), message.size(), flags) {}

  /**
   * @brief Copies a payload from a buffer into the frame.
   * @param id The CAN identifier.
   * @param message The source buffer. It can be null if message_size is zero.
   * @param message_size The number of payload bytes, from zero to eight.
   * @param flags The frame flags.
   * @throws std::invalid_argument if the buffer or frame is invalid.
   */
  inline CanFrame(const uint32_t id, const char* message,
                  const std::size_t message_size, const uint32_t flags)
      : m_id(id), m_flags(flags) {
    if (message_size > m_message.size()) {
      throw std::invalid_argument(
          "Invalid CAN frame: Message length exceeds 8 bytes.");
    }
    if (message_size > 0) {
      if (message == nullptr) {
        throw std::invalid_argument(
            "Invalid CAN frame: Message buffer is null.");
      }
      std::copy_n(message, message_size, m_message.begin());
    }
    m_message_size = static_cast<uint32_t>(message_size);
    validate_frame();
  }

  /**
   * @brief Gets the identifier of the CAN frame.
   * @return The identifier of the CAN frame.
   */
  inline uint32_t id() const noexcept { return m_id; }

  /**
   * @brief Gets the message content of the CAN frame.
   * @return The message content of the CAN frame.
   */
  inline std::vector<char> message() const noexcept {
    return {m_message.begin(), m_message.begin() + m_message_size};
  }

  /**
   * @brief Gets the payload buffer without a copy.
   * @return A pointer that is valid while this frame exists.
   * Use length() for the byte count. Remote requests have no payload.
   */
  inline const char* data() const noexcept { return m_message.data(); }

  /**
   * @brief Gets the flags associated with the CAN frame.
   * @return The flags associated with the CAN frame.
   */
  inline uint32_t flags() const noexcept { return m_flags; }

  /**
   * @brief Checks if the CAN frame is an error frame.
   * @return True if the frame is an error frame, false otherwise.
   */
  inline bool is_error() const noexcept {
    return (m_flags & can_flags::error_frame) != 0;
  }

  /**
   * @brief Checks if the CAN frame is a remote request.
   * @return True if the frame is a remote request, false otherwise.
   */
  inline bool is_remote_request() const noexcept {
    return (m_flags & can_flags::remote_request) != 0;
  }
  /**
   * @brief Checks if the CAN frame uses a standard 11-bit identifier.
   * @return True if the frame uses a standard 11-bit identifier, false
   * otherwise.
   */
  inline bool is_standard_id() const noexcept { return !is_extended_id(); }

  /**
   * @brief Checks if the CAN frame uses an extended 29-bit identifier.
   * @return True if the frame uses an extended 29-bit identifier, false
   * otherwise.
   */
  inline bool is_extended_id() const noexcept {
    return (m_flags & can_flags::extended_id) != 0;
  }

  /**
   * @brief Gets the length of the CAN frame.
   * @return The length of the CAN frame.
   */
  inline uint32_t length() const noexcept {
    return (m_flags & can_flags::remote_request) ? m_requested_length
                                                 : m_message_size;
  }

  /**
   * @brief Comparison operator to check if two CanFrame objects are equal.
   * @param other The other CanFrame object to compare with.
   * @return True if the CanFrame objects are equal, false otherwise.
   */
  inline bool operator==(const CanFrame& other) const noexcept {
    return m_id == other.m_id &&
           m_requested_length == other.m_requested_length &&
           m_message_size == other.m_message_size &&
           m_message == other.m_message && m_flags == other.m_flags;
  }

  /**
   * @brief Comparison operator to check if two CanFrame objects are not equal.
   * @param other The other CanFrame object to compare with.
   * @return True if the CanFrame objects are not equal, false otherwise.
   */
  inline bool operator!=(const CanFrame& other) const noexcept {
    return !(*this == other);
  }

  std::string to_string() const noexcept;

 private:
  const uint32_t m_id{0};  ///< The identifier of the CAN frame.
  const uint32_t m_requested_length{
      0};                           ///< The requested length of the CAN frame.
  std::array<char, 8> m_message{};  ///< The payload buffer.
  uint32_t m_message_size{0};       ///< The number of payload bytes.
  const uint32_t m_flags{};  ///< The flags associated with the CAN frame.

  /**
   * @brief Checks if the identifier fits in 11 bits.
   * @return True if the identifier fits in 11 bits, false otherwise.
   */
  inline bool is_11_bits_id() const noexcept { return is_11_bits(m_id); }

  /**
   * @brief Checks if the identifier fits in 29 bits.
   * @return True if the identifier fits in 29 bits, false otherwise.
   */
  inline bool is_29_bits_id() const noexcept { return is_29_bits(m_id); }

  /**
   * @brief Validates the CAN frame.
   */
  void validate_frame();

  /**
   * @brief Checks if the identifier fits in 11 bits.
   * @param id The identifier to check.
   * @return True if the identifier fits in 11 bits, false otherwise.
   */
  inline static bool is_11_bits(uint32_t id) noexcept {
    return (id & 0xFFFFF800) == 0;
  }
  /**
   * @brief Checks if the identifier fits in 29 bits.
   * @param id The identifier to check.
   * @return True if the identifier fits in 29 bits, false otherwise.
   */
  inline static bool is_29_bits(uint32_t id) noexcept {
    return (id & 0xE0000000) == 0;
  }
};

std::ostream& operator<<(std::ostream& os, const CanFrame& frame) noexcept;

#endif  // SRC_INCLUDE_CANFRAME_H_
