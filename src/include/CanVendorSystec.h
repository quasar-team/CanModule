#ifndef SRC_INCLUDE_CANVENDORSYSTEC_H_
#define SRC_INCLUDE_CANVENDORSYSTEC_H_

#include <atomic>
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
#include <thread>

/**
 * @struct CanVendorSystec
 * @brief Represents a specific implementation of a CanDevice for Systec devices
 * on Windows utilising libraries from USB-CANmodul Utility Disk.
 *
 * This struct provides methods for opening, closing, sending, and receiving CAN
 * frames using the Systec CAN-over-USB interface. It also provides diagnostics
 * information.
 */
struct CanVendorSystec : CanDevice {
  explicit CanVendorSystec(const CanDeviceArguments& args);
  ~CanVendorSystec() { vendor_close(); }
  int SystecRxThread();
  
  private:
  std::atomic<bool> m_receive_thread_flag = true;
  std::atomic<size_t> m_queued_reads;
  int m_module_number;
  int m_channel_number;
  int m_port_number;
  std::thread m_SystecRxThread;

  tUcanHandle get_module_handle() { return m_module_to_handle_map[m_module_number]; }

  CanReturnCode vendor_open() noexcept override;
  CanReturnCode vendor_close() noexcept override;
  CanReturnCode vendor_send(const CanFrame& frame) noexcept override;
  CanDiagnostics vendor_diagnostics() noexcept override;

  CanReturnCode init_can_port();
  static std::mutex m_handles_lock;
  static std::unordered_map<int, tUcanHandle> m_module_to_handle_map;
  static std::unordered_map<int, CanVendorSystec*> m_port_to_vendor_map;

  friend void systec_receive(tUcanHandle UcanHandle_p, DWORD bEvent_p, BYTE bChannel_p, void* pArg_p);
  std::string UsbCanGetErrorText( long err_code );
};

#endif  // SRC_INCLUDE_CANVENDORSYSTEC_H_
