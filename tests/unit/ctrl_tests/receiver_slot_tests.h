
#pragma once

#include "ctrl.h"
#include "ctrl/constants.h"
#include "ctrl/ei_bounds.h"
#include "ctrl/receiver.h"
#include "ctrl/receiver_slots.h"
#include "test_defs.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::string make_length_prefix(uint32_t len) {
  // Same layout as swm_read_length / Erlang <<Len:32/big>>.
  std::string out(4, '\0');
  out[0] = static_cast<char>((len >> 24) & 0xff);
  out[1] = static_cast<char>((len >> 16) & 0xff);
  out[2] = static_cast<char>((len >> 8) & 0xff);
  out[3] = static_cast<char>(len & 0xff);
  return out;
}

}  // namespace

TEST(receiver_slots, slot_ok_bounds) {
  EXPECT_TRUE(swm::util::receiver_slot_ok(0, 7));
  EXPECT_TRUE(swm::util::receiver_slot_ok(6, 7));
  EXPECT_FALSE(swm::util::receiver_slot_ok(7, 7));
  EXPECT_FALSE(swm::util::receiver_slot_ok(255, 7));
  EXPECT_FALSE(swm::util::receiver_slot_ok(0, 0));
}

TEST(receiver_slots, total_ok_cap) {
  EXPECT_TRUE(swm::util::receiver_total_ok(0));
  EXPECT_TRUE(swm::util::receiver_total_ok(7));
  EXPECT_TRUE(swm::util::receiver_total_ok(swm::util::kMaxReceiverDataSlots));
  EXPECT_FALSE(swm::util::receiver_total_ok(static_cast<uint8_t>(swm::util::kMaxReceiverDataSlots + 1)));
  EXPECT_FALSE(swm::util::receiver_total_ok(255));
}

TEST(receiver_slots, slice_len_ok) {
  EXPECT_FALSE(swm::util::receiver_slice_len_ok(0));
  EXPECT_TRUE(swm::util::receiver_slice_len_ok(1));
  EXPECT_TRUE(swm::util::receiver_slice_len_ok(swm::util::kMaxReceiverSliceBytes));
  EXPECT_FALSE(swm::util::receiver_slice_len_ok(swm::util::kMaxReceiverSliceBytes + 1));
  EXPECT_FALSE(swm::util::receiver_slice_len_ok(0xffffffffu));
}

TEST(receiver_slots, ei_list_size_ok) {
  EXPECT_TRUE(swm::util::ei_list_size_ok(0));
  EXPECT_TRUE(swm::util::ei_list_size_ok(swm::util::kMaxEiListElements));
  EXPECT_FALSE(swm::util::ei_list_size_ok(swm::util::kMaxEiListElements + 1));
  EXPECT_FALSE(swm::util::ei_list_size_ok(-1));
}

TEST_F(ctrl, receiver_rejects_high_bit_type) {
  // Wire type 0xFF used to become signed char -1 and pass type >= total.
  std::string payload;
  payload.push_back(static_cast<char>(swm::util::SWM_COMMAND_SCHEDULE));
  payload.push_back(static_cast<char>(7));
  payload.push_back(static_cast<char>(0xff));
  payload += make_length_prefix(1);
  payload.push_back('x');

  std::istringstream istr(payload);
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface>> queue(2);
  swm::util::Receiver receiver;
  ASSERT_NO_THROW(receiver.init(&queue, &istr));
  while (!receiver.finished() && queue.element_count() < queue.size()) {
    std::this_thread::yield();
  }
  ASSERT_EQ(queue.element_count(), 0);
}

TEST_F(ctrl, receiver_rejects_type_equal_total) {
  std::string payload;
  payload.push_back(static_cast<char>(swm::util::SWM_COMMAND_SCHEDULE));
  payload.push_back(static_cast<char>(7));
  payload.push_back(static_cast<char>(7));
  payload += make_length_prefix(1);
  payload.push_back('x');

  std::istringstream istr(payload);
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface>> queue(2);
  swm::util::Receiver receiver;
  ASSERT_NO_THROW(receiver.init(&queue, &istr));
  while (!receiver.finished() && queue.element_count() < queue.size()) {
    std::this_thread::yield();
  }
  ASSERT_EQ(queue.element_count(), 0);
}

TEST_F(ctrl, receiver_rejects_oversized_total) {
  std::string payload;
  payload.push_back(static_cast<char>(swm::util::SWM_COMMAND_SCHEDULE));
  payload.push_back(static_cast<char>(0xff));

  std::istringstream istr(payload);
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface>> queue(2);
  swm::util::Receiver receiver;
  ASSERT_NO_THROW(receiver.init(&queue, &istr));
  while (!receiver.finished() && queue.element_count() < queue.size()) {
    std::this_thread::yield();
  }
  ASSERT_EQ(queue.element_count(), 0);
}

TEST_F(ctrl, receiver_rejects_zero_slice_length) {
  std::string payload;
  payload.push_back(static_cast<char>(swm::util::SWM_COMMAND_SCHEDULE));
  payload.push_back(static_cast<char>(1));
  payload.push_back(static_cast<char>(0));
  payload += make_length_prefix(0);

  std::istringstream istr(payload);
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface>> queue(2);
  swm::util::Receiver receiver;
  ASSERT_NO_THROW(receiver.init(&queue, &istr));
  while (!receiver.finished() && queue.element_count() < queue.size()) {
    std::this_thread::yield();
  }
  ASSERT_EQ(queue.element_count(), 0);
}

TEST_F(ctrl, receiver_rejects_oversized_slice_length) {
  const uint32_t too_big = swm::util::kMaxReceiverSliceBytes + 1;
  std::string payload;
  payload.push_back(static_cast<char>(swm::util::SWM_COMMAND_SCHEDULE));
  payload.push_back(static_cast<char>(1));
  payload.push_back(static_cast<char>(0));
  payload += make_length_prefix(too_big);
  // Do not append ``too_big`` bytes -- rejection must happen before allocate/read.

  std::istringstream istr(payload);
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface>> queue(2);
  swm::util::Receiver receiver;
  ASSERT_NO_THROW(receiver.init(&queue, &istr));
  while (!receiver.finished() && queue.element_count() < queue.size()) {
    std::this_thread::yield();
  }
  ASSERT_EQ(queue.element_count(), 0);
}

TEST_F(ctrl, receiver_accepts_small_valid_slice) {
  // Interrupt ignores payload; proves length check allows a small read.
  std::string payload;
  payload.push_back(static_cast<char>(swm::util::SWM_COMMAND_INTERRUPT));
  payload.push_back(static_cast<char>(1));
  payload.push_back(static_cast<char>(0));
  payload += make_length_prefix(1);
  payload.push_back('x');

  std::istringstream istr(payload);
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface>> queue(2);
  swm::util::Receiver receiver;
  ASSERT_NO_THROW(receiver.init(&queue, &istr));
  while (!receiver.finished() && queue.element_count() < queue.size()) {
    std::this_thread::yield();
  }
  ASSERT_EQ(queue.element_count(), 1);
  ASSERT_EQ(queue.pop()->type(), swm::util::SWM_COMMAND_INTERRUPT);
}
