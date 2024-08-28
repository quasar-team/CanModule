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
  CanReturnCode vendor_open() override;
  CanReturnCode vendor_close() override;
  CanReturnCode vendor_send(const CanFrame &frame) override;
  CanDiagnostics vendor_diagnostics() override;

  void print_anagate_error(AnaUInt32 r);
  static std::mutex m_handles_lock;
  static std::map<int, CanVendorAnagate *> m_handles;

  AnaInt32 m_handle{0};
};

namespace AnagateConstants {
constexpr int extendedId = 1 << 0;
constexpr int remoteRequest = 1 << 1;
constexpr char emptyMessage[8] = {0};
constexpr uint32_t defaultTimeout = 6000;
constexpr unsigned int errorNone = 0;
constexpr unsigned int errorOpenMaxConn = 0x000001;
constexpr unsigned int errorOpCmdFailed = 0x0000FF;
constexpr unsigned int errorTcpipSocket = 0x020000;
constexpr unsigned int errorTcpipNotConnected = 0x030000;
constexpr unsigned int errorTcpipTimeout = 0x040000;
constexpr unsigned int errorTcpipCallNotAllowed = 0x050000;
constexpr unsigned int errorTcpipNotInitialized = 0x060000;
constexpr unsigned int errorInvalidCrc = 0x0A0000;
constexpr unsigned int errorInvalidConf = 0x0B0000;
constexpr unsigned int errorInvalidConfData = 0x0C0000;
constexpr unsigned int errorInvalidDeviceHandle = 0x900000;
constexpr unsigned int errorInvalidDeviceType = 0x910000;
constexpr unsigned int errorCanNack = 0x000220;
constexpr unsigned int errorCanTxError = 0x000221;
constexpr unsigned int errorCanTxBufOverflow = 0x000222;
constexpr unsigned int errorCanTxMloa = 0x000223;
constexpr unsigned int errorCanNoValidBaudrate = 0x000224;
}  // namespace AnagateConstants

#endif  // SRC_INCLUDE_CANVENDORANAGATE_H_
