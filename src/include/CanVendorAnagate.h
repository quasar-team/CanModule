#ifndef SRC_INCLUDE_CANVENDORANAGATE_H_
#define SRC_INCLUDE_CANVENDORANAGATE_H_

#include "AnaGate.h"
#include "CanDevice.h"
#include "CanVendorLoopback.h"

struct CanVendorAnagate : CanDevice {
  explicit CanVendorAnagate(const CanDeviceArguments &configuration)
      : CanDevice("anagate", configuration) {}
  ~CanVendorAnagate() override { vendor_close(); }

 private:
  int vendor_open() override;
  int vendor_close() override;
  int vendor_send(const CanFrame &frame) override;

  AnaInt32 m_handle{0};
};

#endif  // SRC_INCLUDE_CANVENDORANAGATE_H_
