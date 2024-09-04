
#include "CanVersion.h"

#include <gtest/gtest.h>

// Test fixture for CanVersionTest
class CanVersionTest : public ::testing::Test {
 protected:
  // You can define any setup or teardown code here if needed
};

TEST_F(CanVersionTest, VersionNotEmpty) {
  ASSERT_FALSE(CanModule::Version::value.empty());
  ASSERT_FALSE(CanModule::Version::date.empty());
}
