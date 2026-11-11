#include "CanFrame.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

// Test fixture for CanFrame
class CanFrameTest : public ::testing::Test {
 protected:
  // You can define any setup or teardown code here if needed
};

// Test for CanFrame constructor with id
TEST_F(CanFrameTest, ConstructorWithId) {
  uint32_t id = 1;
  std::optional<CanFrame> frameopt;

  frameopt.emplace(CanFrame(2));
  ASSERT_EQ(frameopt.value().id(), 2);

  CanFrame frame1(id);
  frameopt.emplace(frame1);
  CanFrame frame(frameopt.value());
  ASSERT_EQ(frame.id(), id);
  ASSERT_TRUE(frame.message().empty());
  ASSERT_EQ(frame.length(), 0);
  ASSERT_TRUE(frame.is_standard_id());
  ASSERT_FALSE(frame.is_error());
  ASSERT_FALSE(frame.is_remote_request());
}

// Test for CanFrame constructor with id and length 0
TEST_F(CanFrameTest, ConstructorWithIdAndZeroLength) {
  uint32_t id = 1;
  CanFrame frame(id, 0);
  ASSERT_EQ(frame.id(), id);
  ASSERT_TRUE(frame.is_standard_id());
  ASSERT_FALSE(frame.is_error());
  ASSERT_TRUE(frame.is_remote_request());
  ASSERT_EQ(frame.length(), 0);
}

// Test for CanFrame constructor with id and message
TEST_F(CanFrameTest, ConstructorWithIdAndMessage) {
  uint32_t id = 1;
  std::vector<char> message = {'H', 'e', 'l', 'l', 'o'};
  CanFrame frame(id, message);
  CanFrame frame2(1, {'H', 'e', 'l', 'l', 'o'});
  ASSERT_EQ(frame.id(), id);
  ASSERT_EQ(frame.message(), message);
  ASSERT_EQ(frame.message().size(), message.size());
  ASSERT_TRUE(frame.is_standard_id());
  ASSERT_EQ(frame2.id(), id);
  ASSERT_EQ(frame2.message(), message);
  ASSERT_EQ(frame2.message().size(), message.size());
  ASSERT_TRUE(frame2.is_standard_id());
}

// Test for CanFrame constructor with id extended and message
TEST_F(CanFrameTest, ConstructorWithIdExtendedAndMessage) {
  uint32_t id = 1 << 28;
  std::vector<char> message = {'H', 'e', 'l', 'l', 'o'};
  CanFrame frame(id, message);
  CanFrame frame2(1 << 28, {'H', 'e', 'l', 'l', 'o'});
  ASSERT_EQ(frame.id(), id);
  ASSERT_EQ(frame.message(), message);
  ASSERT_EQ(frame.message().size(), message.size());
  ASSERT_TRUE(frame.is_extended_id());
  ASSERT_EQ(frame2.id(), id);
  ASSERT_EQ(frame2.message(), message);
  ASSERT_EQ(frame2.message().size(), message.size());
  ASSERT_TRUE(frame2.is_extended_id());
}

// Test for CanFrame constructor with id too big and message
TEST_F(CanFrameTest, ConstructorWithIdInvalidAndMessage) {
  uint32_t id = 1 << 29;
  std::vector<char> message = {'H', 'e', 'l', 'l', 'o'};
  ASSERT_THROW(CanFrame frame(id, message), std::invalid_argument);
}

TEST_F(CanFrameTest, ConstructorWithIdAndMessageInvalid) {
  uint32_t id = 1 << 29;
  std::vector<char> message = {'H', 'e', 'l', 'l', 'o', ' ',
                               'W', 'o', 'r', 'l', 'd'};
  ASSERT_THROW(CanFrame frame(id, message), std::invalid_argument);
}

// Test for CanFrame constructor with id and requested_length
TEST_F(CanFrameTest, ConstructorWithIdAndRequestedLength) {
  uint32_t id = 1;
  uint32_t requested_length = 8;
  CanFrame frame(id, requested_length);
  ASSERT_EQ(frame.flags(), can_flags::standard_id | can_flags::remote_request);
  ASSERT_EQ(frame.id(), id);
  ASSERT_EQ(frame.flags(), can_flags::standard_id | can_flags::remote_request);
  ASSERT_EQ(frame.length(), requested_length);
  ASSERT_TRUE(frame.message().empty());
  ASSERT_TRUE(frame.is_remote_request());
}

// Test for CanFrame constructor with id, requested_length, and flags
TEST_F(CanFrameTest, ConstructorWithIdRequestedLengthAndFlagsIncomplete) {
  uint32_t id = 1;
  uint32_t requested_length = 8;
  uint32_t flags = can_flags::standard_id;
  ASSERT_THROW(CanFrame frame(id, requested_length, flags),
               std::invalid_argument);
}

TEST_F(CanFrameTest, ConstructorWithIdRequestedLengthAndFlags) {
  uint32_t id = 1;
  uint32_t requested_length = 8;
  uint32_t flags = can_flags::standard_id | can_flags::remote_request;
  CanFrame frame(id, requested_length, flags);
  ASSERT_EQ(frame.length(), 8);
  ASSERT_EQ(frame.id(), id);
}

// Test for CanFrame constructor with id, message, and flags
TEST_F(CanFrameTest, ConstructorWithIdMessageAndFlags) {
  uint32_t id = 1;
  std::vector<char> message = {'H', 'e', 'l', 'l', 'o'};
  uint32_t flags = can_flags::extended_id;
  CanFrame frame(id, message, flags);
  ASSERT_EQ(frame.message(), message);
  ASSERT_EQ(frame.id(), id);
  ASSERT_EQ(frame.flags(), can_flags::extended_id);
}

TEST_F(CanFrameTest, ConstructorWithIdExtendedMessageAndFlagsStandard) {
  uint32_t id = 1 << 28;
  std::vector<char> message = {'H', 'e', 'l', 'l', 'o'};
  uint32_t flags = can_flags::standard_id;
  ASSERT_THROW(CanFrame frame(id, message, flags), std::invalid_argument);
}

TEST_F(CanFrameTest, ConstructorWithIdMessageAndFlagsRTR) {
  uint32_t id = 1 << 28;
  std::vector<char> message = {'H', 'e', 'l', 'l', 'o'};
  uint32_t flags = can_flags::remote_request;
  ASSERT_THROW(CanFrame frame(id, message, flags), std::invalid_argument);
}

TEST_F(CanFrameTest, ConstructorWithIdExt) {
  uint32_t id = 1 << 28;
  CanFrame frame(id);
  ASSERT_EQ(frame.id(), id);
  ASSERT_TRUE(frame.message().empty());
  ASSERT_EQ(frame.length(), 0);
  ASSERT_TRUE(frame.is_extended_id());
  ASSERT_FALSE(frame.is_remote_request());
}

TEST_F(CanFrameTest, TestEqual) {
  uint32_t id = 1 << 28;
  CanFrame frame1(id);
  CanFrame frame2(id);
  ASSERT_EQ(frame1, frame2);
}

TEST_F(CanFrameTest, TestNotEqual) {
  CanFrame frame1(1 << 28);
  CanFrame frame2(1);
  ASSERT_NE(frame1, frame2);
}

TEST_F(CanFrameTest, VectorPayloadLengths) {
  const std::vector<char> bytes = {0, 1, 2, 3,
                                   4, 5, 6, static_cast<char>(0xFF)};
  for (std::size_t size = 0; size <= bytes.size(); ++size) {
    const std::vector<char> payload(bytes.begin(), bytes.begin() + size);
    const CanFrame frame(1, payload);
    EXPECT_EQ(frame.message(), payload);
    EXPECT_EQ(frame.length(), size);
    EXPECT_TRUE(std::equal(payload.begin(), payload.end(), frame.data()));
  }
}

TEST_F(CanFrameTest, PayloadCopiesAreIndependent) {
  std::vector<char> payload = {'a', 'b'};
  const CanFrame frame(1, payload);
  payload[0] = 'x';
  auto message = frame.message();
  message[1] = 'x';
  EXPECT_EQ(frame.message(), (std::vector<char>{'a', 'b'}));
}

TEST_F(CanFrameTest, BufferPayloadOwnsItsBytes) {
  char payload[] = {0, 1, 2, 3, 4, 5, 6, static_cast<char>(0xFF)};
  CanFrame frame(1, payload, sizeof(payload), can_flags::standard_id);
  const auto expected = frame.message();
  payload[0] = 'x';
  const CanFrame copy(frame);
  const CanFrame moved(std::move(frame));
  EXPECT_EQ(copy.message(), expected);
  EXPECT_EQ(moved.message(), expected);
  EXPECT_NE(copy.data(), moved.data());
}

TEST_F(CanFrameTest, RejectsOversizedPayloads) {
  EXPECT_THROW(CanFrame(1, std::vector<char>(9)), std::invalid_argument);
  EXPECT_THROW(CanFrame(1, nullptr, 9, can_flags::standard_id),
               std::invalid_argument);
  EXPECT_THROW(CanFrame(1, nullptr, std::numeric_limits<std::size_t>::max(),
                        can_flags::standard_id),
               std::invalid_argument);
}

TEST_F(CanFrameTest, NullBufferRequiresZeroLength) {
  const CanFrame frame(1, nullptr, 0, can_flags::standard_id);
  EXPECT_TRUE(frame.message().empty());
  EXPECT_EQ(frame.length(), 0);
  EXPECT_THROW(CanFrame(1, nullptr, 1, can_flags::standard_id),
               std::invalid_argument);
}

TEST_F(CanFrameTest, RemoteRequestHasNoPayload) {
  const CanFrame frame(1, 8);
  EXPECT_EQ(frame.length(), 8);
  EXPECT_TRUE(frame.message().empty());
  EXPECT_EQ(frame.to_string(), "001 [8] RTR");
  EXPECT_THROW(CanFrame(1, "a", 1, can_flags::remote_request),
               std::invalid_argument);
}

TEST_F(CanFrameTest, EqualityIncludesPayloadLength) {
  const CanFrame empty(1);
  const CanFrame one(1, std::vector<char>{0});
  const CanFrame two(1, std::vector<char>{0, 0});
  EXPECT_NE(empty, one);
  EXPECT_NE(one, two);
  EXPECT_EQ(one, CanFrame(1, std::vector<char>{0}));
}

TEST_F(CanFrameTest, FormatsOnlyPayloadBytes) {
  const CanFrame frame(1, std::vector<char>{0, static_cast<char>(0xFF)});
  EXPECT_EQ(frame.to_string(), "001 [2] 00 ff");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
