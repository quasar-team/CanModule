#ifndef SRC_INCLUDE_CANDEVICECONFIGURATION_H_
#define SRC_INCLUDE_CANDEVICECONFIGURATION_H_

#include <iostream>
#include <optional>
#include <string>

/**
 * @brief Configuration structure for a CanDevice.
 *
 * This structure holds the configuration details for a CAN device. It supports
 * different configurations for various types of CAN devices, such as SocketCAN
 * and Anagate. Some configuration parameters are optional and may be ignored
 * depending on the type of CAN device being used.
 */
struct CanDeviceConfiguration {
  /**
   * @brief The name of the CAN bus.
   *
   * This parameter is required and used for SocketCAN devices. It is ignored
   * for Anagate devices.
   */
  std::optional<std::string> bus_name;

  /**
   * @brief The port number of the CAN bus.
   *
   * This parameter is required for Anagate devices. It is ignored for SocketCAN
   * devices.
   */
  std::optional<uint32_t> bus_number;

  /**
   * @brief The host address for the CAN device.
   *
   * This parameter is required for Anagate devices. It is ignored for SocketCAN
   * devices.
   */
  std::optional<std::string> host;

  /**
   * @brief The bitrate of the CAN bus.
   *
   * This parameter is optional for both Anagate and SocketCAN devices. It
   * specifies the communication speed of the CAN bus. If not provided, the
   * value of the device is kept.
   */
  std::optional<uint32_t> bitrate;

  /**
   * @brief Enable or disable termination on the CAN bus.
   *
   * This optional parameter is used for Anagate devices to enable or disable
   * termination on the CAN bus. It is ignored for SocketCAN devices. If unset,
   * it won't modify existing termination mode.
   */
  std::optional<bool> enable_termination;

  /**
   * @brief Enable or disable high speed mode on the CAN bus.
   *
   * This optional parameter is used for Anagate devices to enable or disable
   * the high speed mode on the CAN bus. It is ignored for SocketCAN devices.
   * The high speed mode supresses the acknowledgment of sent messages, reducing
   * the load of the Anagate device. If unset, it won't modify existing high
   * speed mode.
   */
  std::optional<bool> high_speed;

  /**
   * @brief The timeout value for CAN operations.
   *
   * This parameter is optional for both Anagate and SocketCAN devices. For
   * Anagate devices, it defaults to 6000 milliseconds. For SocketCAN devices,
   * it specifies the restart-ms value and it defaults to 0.
   */
  std::optional<uint32_t> timeout;

  /**
   * @brief The timeout value for CAN operations.
   *
   * This parameter is optional and only has effect for SocketCAN devices.
   * It is used to specify if we are using a real CAN or a virtual CAN, defaults
   * to false.
   */
  std::optional<bool> vcan;

  /**
   * @brief The sent acknowledgement flag for Anagate.
   *
   * This parameter is optional for Anagate and it defaults to false.
   * If turned on, the Anagate API waits for the message to be acknowledge
   * by the device before ending the call. By default the message is only sent.
   */
  std::optional<uint32_t> sent_acknowledgement;

  std::string to_string() const noexcept;
};

std::ostream &operator<<(std::ostream &os,
                         const CanDeviceConfiguration &config) noexcept;

#endif  // SRC_INCLUDE_CANDEVICECONFIGURATION_H_
