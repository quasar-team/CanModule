#include "CanDevice.h"

#include <gtest/gtest.h>

#include <iostream>
#include <stdexcept>
#include <thread>  // NOLINT
#include <vector>

struct TestableCanDevice : CanDevice {
  TestableCanDevice(std::string_view vendor_name,
                    const CanDeviceArguments& args) noexcept
      : CanDevice(vendor_name, args) {}

  using CanDevice::notify_error;
  using CanDevice::received;

  CanReturnCode vendor_open() noexcept override {
    return CanReturnCode::success;
  }
  CanReturnCode vendor_close() noexcept override {
    return CanReturnCode::success;
  }
  CanReturnCode vendor_send(const CanFrame&) noexcept override {
    return CanReturnCode::success;
  }
  CanDiagnostics vendor_diagnostics() noexcept override {
    return CanDiagnostics{};
  }
};

// Test fixture for CanFrame
class CanDeviceTest : public ::testing::Test {
 protected:
  // You can define any setup or teardown code here if needed
};

// Test for CanFrame constructor with id and message
TEST_F(CanDeviceTest, CreationLoopbackDevice) {
  auto dummy_cb_ = [](const CanFrame& frame) { return; };
  auto myDevice = CanDevice::create(
      "loopback",
      CanDeviceArguments{CanDeviceConfiguration{{{"bus_name", "dummy"}}},
                         dummy_cb_});
  ASSERT_NE(myDevice, nullptr);
  ASSERT_EQ(myDevice->vendor_name(), "loopback");
  ASSERT_EQ(myDevice->args().config.bus_name.value(), "dummy");
}

// Test for CanFrame constructor with id and message
TEST_F(CanDeviceTest, LoopbackDeviceMessageTransmission) {
  std::vector<CanFrame> outFrames;
  std::vector<CanFrame> inFrames;

  auto dummy_cb_ = [&inFrames](const CanFrame& frame) {
    inFrames.push_back(frame);
  };
  auto myDevice = CanDevice::create(
      "loopback",
      CanDeviceArguments{CanDeviceConfiguration{{{"bus_name", "dummy"}}},
                         dummy_cb_});

  for (uint32_t i = 0; i < 10; ++i) {
    outFrames.push_back(CanFrame{i});
  }

  myDevice->send(outFrames);

  ASSERT_EQ(outFrames.size(), 10);
  ASSERT_EQ(inFrames.size(), 10);

  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(outFrames[i], inFrames[i]);
    ASSERT_EQ(outFrames[i].id(), inFrames[i].id());
    ASSERT_EQ(outFrames[i].message(), inFrames[i].message());
    ASSERT_EQ(outFrames[i].is_remote_request(),
              inFrames[i].is_remote_request());
  }
}

// Test for error notification
TEST_F(CanDeviceTest, OnErrorCallbackIsInvoked) {
  CanReturnCode captured_code = CanReturnCode::success;
  bool called = false;
  auto on_error_cb = [&](CanReturnCode code) {
    called = true;
    captured_code = code;
  };

  TestableCanDevice device{
      "test",
      CanDeviceArguments{CanDeviceConfiguration{{{"bus_name", "dummy"}}},
                         [](const CanFrame&) {}, on_error_cb}};
  device.notify_error(CanReturnCode::disconnected);

  ASSERT_TRUE(called);
  ASSERT_EQ(captured_code, CanReturnCode::disconnected);
}

TEST_F(CanDeviceTest, ThrowingCallbacksAreHandled) {
  bool receiver_called = false;
  auto receiver_cb = [&](const CanFrame&) {
    receiver_called = true;
    throw std::runtime_error("error");
  };

  bool on_error_called = false;
  auto on_error_cb = [&](CanReturnCode) {
    on_error_called = true;
    throw std::runtime_error("error");
  };

  TestableCanDevice device{
      "test",
      CanDeviceArguments{CanDeviceConfiguration{{{"bus_name", "dummy"}}},
                         receiver_cb, on_error_cb}};

  ASSERT_NO_THROW(device.received(CanFrame{0}));
  ASSERT_TRUE(receiver_called);

  ASSERT_NO_THROW(device.notify_error(CanReturnCode::disconnected));
  ASSERT_TRUE(on_error_called);
}
