#include "CanDevice.h"

#include <gtest/gtest.h>

// Test fixture for CanFrame
class CanDeviceTest : public ::testing::Test {
 protected:
  // You can define any setup or teardown code here if needed
};

// Test for CanFrame constructor with id and message
TEST_F(CanDeviceTest, DummyDeviceCreation) {
  auto myDevice = CanDevice::create("dummy", "Hello World");
  ASSERT_NE(myDevice, nullptr);
  // ASSERT_EQ(myDevice->vendor_name(), "dummy");
}
