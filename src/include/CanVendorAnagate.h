#ifndef SRC_INCLUDE_CANVENDORANAGATE_H_
#define SRC_INCLUDE_CANVENDORANAGATE_H_

#include <map>
#include <mutex>  //NOLINT

#include "AnaGateDllCan.h"
#include "CanDevice.h"
#include "CanVendorLoopback.h"

struct CanVendorAnagate : CanDevice {
  friend void anagate_receive(AnaInt32 nIdentifier, const char *pcBuffer,
                              AnaInt32 nBufferLen, AnaInt32 nFlags,
                              AnaInt32 hHandle);

  explicit CanVendorAnagate(const CanDeviceArguments &configuration)
      : CanDevice("anagate", configuration) {}
  ~CanVendorAnagate() override { vendor_close(); }

 private:
  int vendor_open() override;
  int vendor_close() override;
  int vendor_send(const CanFrame &frame) override;

  static std::mutex m_handles_lock;
  static std::map<int, CanVendorAnagate *> m_handles;

  AnaInt32 m_handle{0};
};

namespace AnagateConstants {
constexpr int EXTENDED_ID = 1 << 0;
constexpr int REMOTE_REQUEST = 1 << 1;
constexpr char empty_message[8] = {0};
constexpr uint32_t default_timeout = 6000;
}  // namespace AnagateConstants

#endif  // SRC_INCLUDE_CANVENDORANAGATE_H_
