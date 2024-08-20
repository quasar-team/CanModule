#ifndef SRC_INCLUDE_CANDEVICECONFIGURATION_H_
#define SRC_INCLUDE_CANDEVICECONFIGURATION_H_

#include <optional>
#include <string>

/**
 * @brief Configuration structure for a CAN device.
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
   * This parameter is used for SocketCAN devices. It is ignored for Anagate
   * devices.
   */
  std::optional<std::string> bus_name;

  /**
   * @brief The address of the CAN bus.
   *
   * This parameter is used for Anagate devices. It is ignored for SocketCAN
   * devices.
   */
  std::optional<uint32_t> bus_address;

  /**
   * @brief The host address for the CAN device.
   *
   * This parameter is used for Anagate devices. It is ignored for SocketCAN
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
   * This parameter is used for Anagate devices to enable or disable termination
   * on the CAN bus. It is ignored for SocketCAN devices.
   */
  std::optional<bool> enable_termination;

  /**
   * @brief The timeout value for CAN operations.
   *
   * This parameter is optional for both Anagate and SocketCAN devices. For
   * Anagate devices, it defaults to 6000 milliseconds. For SocketCAN devices,
   * it specifies the restart-ms value for the SocketCAN API.
   */
  std::optional<uint32_t> timeout;
};

#endif  // SRC_INCLUDE_CANDEVICECONFIGURATION_H_
