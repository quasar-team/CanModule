#include "CanDevice.h"

#include <gtest/gtest.h>

#include <iostream>

#include "CanVendorLoopback.h"

// Test fixture for CanFrame
class CanDeviceTest : public ::testing::Test {
 protected:
  // You can define any setup or teardown code here if needed
};

// Test for CanFrame constructor with id and message
TEST_F(CanDeviceTest, CreationLoopbackDevice) {
  auto dummy_cb_ = [](const CanFrame& frame) { return; };
  auto myDevice =
      CanDevice::create("loopback", CanDeviceConfig{"dummy config", dummy_cb_});
  // ASSERT_NE(myDevice, nullptr);
  // ASSERT_EQ(myDevice->vendor_name(), "loopback");
  // ASSERT_EQ(myDevice->configuration().vendor_config, "dummy config");
}

// Test for CanFrame constructor with id and message
TEST_F(CanDeviceTest, LoopbackDeviceMessageTransmission) {
  std::vector<CanFrame> outFrames;
  std::vector<CanFrame> inFrames;

  auto dummy_cb_ = [&inFrames](const CanFrame& frame) {
    inFrames.push_back(frame);
  };
  auto myDevice =
      CanDevice::create("loopback", CanDeviceConfig{"dummy config", dummy_cb_});

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
