#include "CanDeviceConfiguration.h"

#include <gtest/gtest.h>

#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class CanDeviceConfigurationTest : public ::testing::Test {};

TEST_F(CanDeviceConfigurationTest, DefaultConstructorLeavesEverythingUnset) {
  const CanDeviceConfiguration config;

  ASSERT_FALSE(config.bus_name.has_value());
  ASSERT_FALSE(config.bus_number.has_value());
  ASSERT_FALSE(config.host.has_value());
  ASSERT_FALSE(config.bitrate.has_value());
  ASSERT_FALSE(config.enable_termination.has_value());
  ASSERT_FALSE(config.high_speed.has_value());
  ASSERT_FALSE(config.timeout.has_value());
  ASSERT_FALSE(config.vcan.has_value());
  ASSERT_FALSE(config.sent_acknowledgement.has_value());
}

TEST_F(CanDeviceConfigurationTest, AssignsEveryParameterOfTheMap) {
  const CanDeviceConfiguration config = CanDeviceConfiguration::from_map({
      {"bus_name", "can0"},
      {"bus_number", "2"},
      {"host", "127.0.0.1"},
      {"bitrate", "125000"},
      {"enable_termination", "true"},
      {"high_speed", "false"},
      {"timeout", "6000"},
      {"vcan", "true"},
      {"sent_acknowledgement", "1"},
  });

  ASSERT_EQ(config.bus_name.value(), "can0");
  ASSERT_EQ(config.bus_number.value(), 2);
  ASSERT_EQ(config.host.value(), "127.0.0.1");
  ASSERT_EQ(config.bitrate.value(), 125000);
  ASSERT_TRUE(config.enable_termination.value());
  ASSERT_FALSE(config.high_speed.value());
  ASSERT_EQ(config.timeout.value(), 6000);
  ASSERT_TRUE(config.vcan.value());
  ASSERT_EQ(config.sent_acknowledgement.value(), 1);
}

TEST_F(CanDeviceConfigurationTest, LeavesTheAbsentParametersUnset) {
  const CanDeviceConfiguration config =
      CanDeviceConfiguration::from_map({{"bus_name", "can0"}});

  ASSERT_EQ(config.bus_name.value(), "can0");
  ASSERT_FALSE(config.bus_number.has_value());
  ASSERT_FALSE(config.host.has_value());
  ASSERT_FALSE(config.bitrate.has_value());
  ASSERT_FALSE(config.enable_termination.has_value());
  ASSERT_FALSE(config.high_speed.has_value());
  ASSERT_FALSE(config.timeout.has_value());
  ASSERT_FALSE(config.vcan.has_value());
  ASSERT_FALSE(config.sent_acknowledgement.has_value());
}

TEST_F(CanDeviceConfigurationTest, AssignsEveryParameterPositionally) {
  const CanDeviceConfiguration config{"can0", 2,    "127.0.0.1", 125000, true,
                                      false,  6000, true,        1};

  ASSERT_EQ(config.bus_name.value(), "can0");
  ASSERT_EQ(config.bus_number.value(), 2);
  ASSERT_EQ(config.host.value(), "127.0.0.1");
  ASSERT_EQ(config.bitrate.value(), 125000);
  ASSERT_TRUE(config.enable_termination.value());
  ASSERT_FALSE(config.high_speed.value());
  ASSERT_EQ(config.timeout.value(), 6000);
  ASSERT_TRUE(config.vcan.value());
  ASSERT_EQ(config.sent_acknowledgement.value(), 1);
}

TEST_F(CanDeviceConfigurationTest, LeavesTrailingPositionalParametersUnset) {
  const CanDeviceConfiguration config{"can0", 2};

  ASSERT_EQ(config.bus_name.value(), "can0");
  ASSERT_EQ(config.bus_number.value(), 2);
  ASSERT_FALSE(config.host.has_value());
  ASSERT_FALSE(config.bitrate.has_value());
  ASSERT_FALSE(config.enable_termination.has_value());
  ASSERT_FALSE(config.high_speed.has_value());
  ASSERT_FALSE(config.timeout.has_value());
  ASSERT_FALSE(config.vcan.has_value());
  ASSERT_FALSE(config.sent_acknowledgement.has_value());
}

TEST_F(CanDeviceConfigurationTest, RejectsUnknownParameters) {
  ASSERT_THROW(CanDeviceConfiguration::from_map({{"buss_name", "can0"}}),
               std::invalid_argument);
  ASSERT_THROW(CanDeviceConfiguration::from_map({{"vendor", "socketcan"}}),
               std::invalid_argument);
  ASSERT_THROW(CanDeviceConfiguration::from_map({{"", ""}}),
               std::invalid_argument);
}

TEST_F(CanDeviceConfigurationTest, RejectsValuesOfTheWrongType) {
  const std::vector<std::pair<std::string, std::string>> invalid_values = {
      {"bus_number", ""},
      {"bus_number", "-1"},
      {"bus_number", "+1"},
      {"bus_number", "1.5"},
      {"bus_number", "12a"},
      {"bus_number", "0x2"},
      {"bus_number", " 2 "},
      {"bitrate", "4294967296"},
      {"bitrate", "99999999999999999999"},
      {"enable_termination", ""},
      {"enable_termination", "yes"},
      {"high_speed", "TRUE"},
      {"vcan", "1"},
      {"sent_acknowledgement", "abc"},
  };

  for (const auto& [key, value] : invalid_values) {
    ASSERT_THROW(CanDeviceConfiguration::from_map({{key, value}}),
                 std::invalid_argument)
        << "Expected '" << value << "' to be invalid for '" << key << "'";
  }
}
