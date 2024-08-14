/**
 * @file CanFrame.h
 * @brief Defines the CanFrame class and related constants for CAN
 * communication.
 */

#ifndef SRC_INCLUDE_CANFRAME_H_
#define SRC_INCLUDE_CANFRAME_H_

#include <cstdint>
#include <vector>

/**
 * @namespace CanFlags
 * @brief Namespace containing flag constants for CAN frames.
 */
namespace CanFlags {
constexpr int STANDARD_ID = 0 << 0;     ///< Standard 11-bit identifier
constexpr int EXTENDED_ID = 1 << 0;     ///< Extended 29-bit identifier
constexpr int ERROR_FRAME = 1 << 1;     ///< Error frame flag
constexpr int REMOTE_REQUEST = 1 << 2;  ///< Remote request flag
};  // namespace CanFlags

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
   */
  CanFrame(const uint32_t id, const uint32_t requested_length,
           const uint32_t flags)
      : m_id(id), m_requested_length(requested_length), m_flags(flags) {
    if (!is_remote_request()) {
      return;
    }
    validate_frame();
  }

  /**
   * @brief Constructs a CanFrame with specified ID and requested length.
   * @param id The identifier of the CAN frame.
   * @param requested_length The requested length of the CAN frame.
   */
  CanFrame(const uint32_t id, const uint32_t requested_length)
      : CanFrame(id, requested_length,
                 CanFlags::REMOTE_REQUEST |
                     (is_11_bits(id) ? CanFlags::STANDARD_ID
                                     : CanFlags::EXTENDED_ID)) {}
  /**
   * @brief Constructs a CanFrame with specified ID.
   * @param id The identifier of the CAN frame.
   */
  explicit CanFrame(const uint32_t id) : CanFrame(id, 0) {}
  /**
   * @brief Constructs a CanFrame with specified ID and message.
   * @param id The identifier of the CAN frame.
   * @param message The message content of the CAN frame.
   */
  CanFrame(const uint32_t id, const std::vector<char>& message)
      : CanFrame(
            id, message,
            is_11_bits(id) ? CanFlags::STANDARD_ID : CanFlags::EXTENDED_ID) {}
  /**
   * @brief Constructs a CanFrame with specified ID, message, and flags.
   * @param id The identifier of the CAN frame.
   * @param message The message content of the CAN frame.
   * @param flags The flags associated with the CAN frame.
   */
  CanFrame(const uint32_t id, const std::vector<char>& message,
           const uint32_t flags)
      : m_id(id), m_message(message), m_flags(flags) {
    validate_frame();
  }

  /**
   * @brief Checks if the CAN frame is valid.
   * @return True if the frame is valid, false otherwise.
   */
  inline bool is_valid() const { return m_valid; }

  /**
   * @brief Gets the identifier of the CAN frame.
   * @return The identifier of the CAN frame.
   */
  inline uint32_t id() const { return m_id; }

  /**
   * @brief Gets the message content of the CAN frame.
   * @return The message content of the CAN frame.
   */
  inline std::vector<char> message() const { return m_message; }

  /**
   * @brief Gets the flags associated with the CAN frame.
   * @return The flags associated with the CAN frame.
   */
  inline uint32_t flags() const { return m_flags; }

  /**
   * @brief Checks if the CAN frame is an error frame.
   * @return True if the frame is an error frame, false otherwise.
   */
  inline bool is_error() const {
    return (m_flags & CanFlags::ERROR_FRAME) != 0;
  }

  /**
   * @brief Checks if the CAN frame is a remote request.
   * @return True if the frame is a remote request, false otherwise.
   */
  inline bool is_remote_request() const {
    return (m_flags & CanFlags::REMOTE_REQUEST) != 0;
  }
  /**
   * @brief Checks if the CAN frame uses a standard 11-bit identifier.
   * @return True if the frame uses a standard 11-bit identifier, false
   * otherwise.
   */
  inline bool is_standard_id() const { return !is_extended_id(); }

  /**
   * @brief Checks if the CAN frame uses an extended 29-bit identifier.
   * @return True if the frame uses an extended 29-bit identifier, false
   * otherwise.
   */
  inline bool is_extended_id() const {
    return (m_flags & CanFlags::EXTENDED_ID) != 0;
  }

  /**
   * @brief Gets the length of the CAN frame.
   * @return The length of the CAN frame.
   */
  inline uint32_t length() const {
    static_assert(sizeof(std::vector<char>::size_type) > sizeof(uint32_t),
                  "m_message is larger than uint32_t");

    return (m_flags & CanFlags::REMOTE_REQUEST)
               ? m_requested_length
               : static_cast<uint32_t>(m_message.size());
  }

  /**
   * @brief Comparison operator to check if two CanFrame objects are equal.
   * @param other The other CanFrame object to compare with.
   * @return True if the CanFrame objects are equal, false otherwise.
   */
  bool operator==(const CanFrame& other) const {
    return m_id == other.m_id &&
           m_requested_length == other.m_requested_length &&
           m_message == other.m_message && m_flags == other.m_flags &&
           m_valid == other.m_valid;
  }

  /**
   * @brief Comparison operator to check if two CanFrame objects are not equal.
   * @param other The other CanFrame object to compare with.
   * @return True if the CanFrame objects are not equal, false otherwise.
   */
  bool operator!=(const CanFrame& other) const { return !(*this == other); }

 private:
  const uint32_t m_id{0};  ///< The identifier of the CAN frame.
  const uint32_t m_requested_length{
      0};  ///< The requested length of the CAN frame.
  const std::vector<char> m_message;  ///< The message content of the CAN frame.
  const uint32_t m_flags{};  ///< The flags associated with the CAN frame.
  bool m_valid{false};       ///< Indicates if the CAN frame is valid.

  /**
   * @brief Checks if the identifier fits in 11 bits.
   * @return True if the identifier fits in 11 bits, false otherwise.
   */
  bool is_11_bits_id() const { return is_11_bits(m_id); }

  /**
   * @brief Checks if the identifier fits in 29 bits.
   * @return True if the identifier fits in 29 bits, false otherwise.
   */
  bool is_29_bits_id() const { return is_29_bits(m_id); }

  /**
   * @brief Validates the CAN frame.
   */
  void validate_frame();

  /**
   * @brief Checks if the identifier fits in 11 bits.
   * @param id The identifier to check.
   * @return True if the identifier fits in 11 bits, false otherwise.
   */
  static bool is_11_bits(uint32_t id) { return (id & 0xFFFFF800) == 0; }
  /**
   * @brief Checks if the identifier fits in 29 bits.
   * @param id The identifier to check.
   * @return True if the identifier fits in 29 bits, false otherwise.
   */
  static bool is_29_bits(uint32_t id) { return (id & 0xE0000000) == 0; }
};

#endif  // SRC_INCLUDE_CANFRAME_H_
