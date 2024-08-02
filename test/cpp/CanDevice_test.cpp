#include "CanDevice.h"

#include <gtest/gtest.h>

#include <iostream>

#include "CanVendorDummy.h"

// Test fixture for CanFrame
class CanDeviceTest : public ::testing::Test {
 protected:
  // You can define any setup or teardown code here if needed
};

// Test for CanFrame constructor with id and message
TEST_F(CanDeviceTest, CreationDummyDevice) {
  auto myDevice = CanDevice::create("dummy", "dummy config");
  ASSERT_NE(myDevice, nullptr);
  ASSERT_EQ(myDevice->vendor_name(), "dummy");
  ASSERT_EQ(myDevice->configuration(), "dummy config");
}

// Test for CanFrame constructor with id and message
TEST_F(CanDeviceTest, DummyDeviceMessageTransmission) {
  auto myDevice = CanDevice::create("dummy", "dummy config");
  std::vector<CanFrame> outFrames;
  std::vector<CanFrame> inFrames;

  for (uint32_t i = 0; i < 10; ++i) {
    outFrames.push_back(CanFrame{i});
  }

  myDevice->open(
      [&inFrames](const CanFrame& frame) { inFrames.push_back(frame); });

  // Generate Google test assertions to check if the transmitted frames are
  // received correctly for (int i = 0; i < 10; ++i) {
  //   ASSERT_EQ(outFrames[i].id(), inFrames[i].id());
  // }
}
