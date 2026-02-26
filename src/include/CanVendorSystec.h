#ifndef SRC_INCLUDE_CANVENDORSYSTEC_H_
#define SRC_INCLUDE_CANVENDORSYSTEC_H_

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
  static DWORD WINAPI SystecRxThread(LPVOID pCanVendorSystec);
  
  private:
  bool m_receive_thread_flag = true;
  tUcanHandle m_UcanHandle;
  int m_module_number;
  int m_channel_number;
  HANDLE m_receive_thread_handle;
  DWORD m_receive_thread_id;
  CanReturnCode vendor_open() noexcept override;
  CanReturnCode vendor_close() noexcept override;
  CanReturnCode vendor_send(const CanFrame& frame) noexcept override;
  CanDiagnostics vendor_diagnostics() noexcept override;
 
  CanReturnCode init_can_port();
  static std::mutex m_handles_lock;

  inline void map_module_to_handle(int module, tUcanHandle handle) { m_handle_map[module] = handle; }
  inline int erase_module_handle(int module) { return m_handle_map.erase(module); }
  
  // TODO i don't like this too much
  inline static std::unordered_map<int, tUcanHandle> m_handle_map = {};
};

#endif  // SRC_INCLUDE_CANVENDORSYSTEC_H_
