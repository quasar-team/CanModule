#ifndef SRC_INCLUDE_CANDEVICE_H_
#define SRC_INCLUDE_CANDEVICE_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CanDeviceArguments.h"
#include "CanDiagnostics.h"
#include "CanFrame.h"

struct CanDevice {
  int open();
  int close();
  int send(const CanFrame& frame);
  std::vector<int> send(const std::vector<CanFrame>& frames);
  CanDiagnostics diagnostics();

  inline std::string vendor_name() const { return m_vendor; }
  inline const CanDeviceArguments& args() const { return m_args; }
  virtual ~CanDevice() = default;

  static std::unique_ptr<CanDevice> create(
      std::string_view vendor, const CanDeviceArguments& configuration);

 protected:
  CanDevice(std::string_view vendor_name, const CanDeviceArguments& args)
      : m_vendor{vendor_name}, m_args{args} {}

  virtual int vendor_open() = 0;
  virtual int vendor_close() = 0;
  virtual int vendor_send(const CanFrame& frame) = 0;
  virtual CanDiagnostics vendor_diagnostics() = 0;

  void received(const CanFrame& frame) { m_args.receiver(frame); }

 private:
  const std::string m_vendor;
  const CanDeviceArguments m_args;
};

#endif  // SRC_INCLUDE_CANDEVICE_H_
