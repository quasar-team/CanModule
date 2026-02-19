#ifndef SRC_INCLUDE_CANVENDORSOCKETCANSYSTEC_H_
#define SRC_INCLUDE_CANVENDORSOCKETCANSYSTEC_H_

#include <memory>
#include "tchar.h"
#include "Winsock2.h"
#include "windows.h"
#include <string>
#include "usbcan32.h"
#include <cstdint>
#include <map>
#include <mutex>  //NOLINT
#include "CanDiagnostics.h"
#include "CanVendorLoopback.h"
#include "CanDevice.h"
#include <unordered_map>

/**
 * @struct CanVendorSystec
 * @brief Represents a SocketCAN Systec specific implementation of a CanDevice.
 *
 * This class provides the implementation for interacting with a SocketCAN
 * Systec device. It extends the CanVendorSocketCan class and overrides the
 * necessary methods to open, close, and send CAN frames using the SocketCAN
 * interface. It provides a custom mechanishm for handling BUS_OFF errors,
 * restarting the interface instead of using the built-in restart mechanism of
 * SocketCan due to a kernel-panic bug on Systec linux module.
 */
struct CanVendorSystec : CanDevice {
  explicit CanVendorSystec(const CanDeviceArguments& args);
  ~CanVendorSystec() { vendor_close(); }
  // should it be a friend or static? STCanScan.h uses a static...
  friend DWORD WINAPI CanScanControlThread(LPVOID pCanVendorSystec);
  
  private:
  bool m_CanScanThreadShutdownFlag = true;
  tUcanHandle m_UcanHandle;
  int m_moduleNumber;
  int m_channelNumber;
  HANDLE m_hReceiveThread;
  DWORD m_idReceiveThread;
  CanReturnCode vendor_open() noexcept override;
  CanReturnCode vendor_close() noexcept override;
  CanReturnCode vendor_send(const CanFrame& frame) noexcept override;
  CanDiagnostics vendor_diagnostics() noexcept override;
 
  CanReturnCode init_can_port();
  // CanReturnCode close_can_port();
 
  
  // TODO i don't like this too much
  inline static std::unordered_map<int, tUcanHandle> m_handleMap = {};

  // std::unique_ptr<CanDevice> m_can_vendor_socketcan;
};

#endif  // SRC_INCLUDE_CANVENDORSOCKETCANSYSTEC_H_
