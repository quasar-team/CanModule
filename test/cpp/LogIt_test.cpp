
#include "LogIt.h"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>  // NOLINT

#include "CanLogIt.h"

// Define a lambda that sleep for 2 seconds and returns "Hello World!"
auto slowFunction = []() {
  std::this_thread::sleep_for(std::chrono::seconds(2));
  return "Hello World!";
};

// Test fixture for CanFrame
class LogItTest : public ::testing::Test {
 protected:
  // You can define any setup or teardown code here if needed
};

TEST_F(LogItTest, LogItSetup) {
  testing::internal::CaptureStdout();
  LOG(Log::INF, CanLogIt::h()) << "Hello World!";
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find(", INF, CanModule] Hello World!"), std::string::npos);
}

TEST_F(LogItTest, LogItSlowFunctionInfo) {
  auto start = std::chrono::high_resolution_clock::now();
  LOG(Log::INF, CanLogIt::h()) << "Entering slowFunction " << slowFunction();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  EXPECT_NEAR(elapsed.count(), 2.0, 0.2);
}

TEST_F(LogItTest, LogItSlowFunctionDebug) {
  auto start = std::chrono::high_resolution_clock::now();
  LOG(Log::DBG, CanLogIt::h())
      << slowFunction() << "Entering slowFunction " << slowFunction();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  EXPECT_NEAR(elapsed.count(), 0.0, 0.1);
}
