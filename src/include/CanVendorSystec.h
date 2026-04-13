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
#include <optional>
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
  static std::string_view UsbCanGetErrorText( long err_code );
  static std::string UsbCanGetStatusText( long err_code );

  private:
  std::atomic<bool> m_module_in_use {false};
  std::atomic<size_t> m_queued_reads;
  int m_module_number;
  int m_channel_number;
  int m_port_number;
  DWORD m_baud_rate;
  std::thread m_SystecRxThread;

  std::optional<tUcanHandle> get_module_handle() {
    if (auto mapping = m_module_to_handle_map.find(m_module_number); mapping != m_module_to_handle_map.end()) {
      return mapping->second;
    }
    return std::nullopt;
  }

  CanReturnCode vendor_open() noexcept override;
  CanReturnCode vendor_close() noexcept override;
  CanReturnCode vendor_send(const CanFrame& frame) noexcept override;
  CanDiagnostics vendor_diagnostics() noexcept override;

  CanReturnCode init_can_port();
  static std::mutex m_handles_lock;
  static std::unordered_map<int, tUcanHandle> m_module_to_handle_map;
  static std::unordered_map<int, CanVendorSystec*> m_port_to_vendor_map;

  friend void systec_receive(tUcanHandle UcanHandle_p, DWORD bEvent_p, BYTE bChannel_p, void* pArg_p);

  CanReturnCode deinit_channel() noexcept;
};

#endif  // SRC_INCLUDE_CANVENDORSYSTEC_H_
