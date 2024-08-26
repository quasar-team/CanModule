#ifndef SRC_INCLUDE_CANVENDORANAGATE_H_
#define SRC_INCLUDE_CANVENDORANAGATE_H_

#include <map>
#include <mutex>  //NOLINT

#include "AnaGateDllCan.h"
#include "CanDevice.h"
#include "CanDiagnostics.h"
#include "CanVendorLoopback.h"

/**
 * @brief A concrete implementation of CanDevice for the Anagate CAN interface.
 *
 * This class provides methods for opening, closing, sending, and receiving CAN
 * frames using the Anagate CAN interface. It also provides diagnostics
 * information.
 */
struct CanVendorAnagate : CanDevice {
  friend void anagate_receive(AnaInt32 nIdentifier, const char *pcBuffer,
                              AnaInt32 nBufferLen, AnaInt32 nFlags,
                              AnaInt32 hHandle);

  /**
   * @brief Constructor for the CanVendorAnagate class.
   *
   * Initializes the CanVendorAnagate object with the provided configuration.
   *
   * @param configuration A constant reference to the CanDeviceArguments object,
   * which contains configuration parameters for the CAN device.
   */
  explicit CanVendorAnagate(const CanDeviceArguments &configuration)
      : CanDevice("anagate", configuration) {}
  ~CanVendorAnagate() override { vendor_close(); }

 private:
  int vendor_open() override;
  int vendor_close() override;
  int vendor_send(const CanFrame &frame) override;
  CanDiagnostics vendor_diagnostics() override;

  void print_anagate_error(AnaUInt32 r);
  static std::mutex m_handles_lock;
  static std::map<int, CanVendorAnagate *> m_handles;

  AnaInt32 m_handle{0};
};

namespace AnagateConstants {
constexpr int EXTENDED_ID = 1 << 0;
constexpr int REMOTE_REQUEST = 1 << 1;
constexpr char empty_message[8] = {0};
constexpr uint32_t default_timeout = 6000;
constexpr unsigned int ERROR_NONE = 0;
constexpr unsigned int ERROR_OPEN_MAX_CONN = 0x000001;
constexpr unsigned int ERROR_OP_CMD_FAILED = 0x0000FF;
constexpr unsigned int ERROR_TCPIP_SOCKET = 0x020000;
constexpr unsigned int ERROR_TCPIP_NOTCONNECTED = 0x030000;
constexpr unsigned int ERROR_TCPIP_TIMEOUT = 0x040000;
constexpr unsigned int ERROR_TCPIP_CALLNOTALLOWED = 0x050000;
constexpr unsigned int ERROR_TCPIP_NOT_INITIALIZED = 0x060000;
constexpr unsigned int ERROR_INVALID_CRC = 0x0A0000;
constexpr unsigned int ERROR_INVALID_CONF = 0x0B0000;
constexpr unsigned int ERROR_INVALID_CONF_DATA = 0x0C0000;
constexpr unsigned int ERROR_INVALID_DEVICE_HANDLE = 0x900000;
constexpr unsigned int ERROR_INVALID_DEVICE_TYPE = 0x910000;
constexpr unsigned int ERROR_CAN_NACK = 0x000220;
constexpr unsigned int ERROR_CAN_TX_ERROR = 0x000221;
constexpr unsigned int ERROR_CAN_TX_BUF_OVERLOW = 0x000222;
constexpr unsigned int ERROR_CAN_TX_MLOA = 0x000223;
constexpr unsigned int ERROR_CAN_NO_VALID_BAUDRATE = 0x000224;
}  // namespace AnagateConstants

#endif  // SRC_INCLUDE_CANVENDORANAGATE_H_
