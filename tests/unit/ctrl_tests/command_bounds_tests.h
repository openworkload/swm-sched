
#pragma once

#include "ctrl.h"
#include "ctrl/commands.h"
#include "ctrl/command_context.h"
#include "ctrl/constants.h"
#include "ctrl/ei_bounds.h"
#include "ctrl/receiver_slots.h"
#include "test_defs.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

class ScheduleCommandTestAccess : public swm::util::ScheduleCommand {
 public:
  using swm::util::ScheduleCommand::ScheduleCommand;
  using swm::util::ScheduleCommand::init;
};

}  // namespace

namespace {

// ETF: version + NIL_EXT (empty term / empty list).
std::string etf_version_nil() {
  return std::string("\x83\x6a", 2);
}

// ETF: version + LIST_EXT with given element count and no elements (invalid body, for header check).
std::string etf_version_list_header(uint32_t list_size) {
  std::string out;
  out.push_back(static_cast<char>(131));  // ERLANG_BINARY_FORMAT_VERSION / VERSION_MAGIC
  out.push_back(static_cast<char>(108));  // ERL_LIST_EXT
  out.push_back(static_cast<char>((list_size >> 24) & 0xff));
  out.push_back(static_cast<char>((list_size >> 16) & 0xff));
  out.push_back(static_cast<char>((list_size >> 8) & 0xff));
  out.push_back(static_cast<char>(list_size & 0xff));
  return out;
}

swm::util::ReceiverSlice slice_from_string(const std::string &s) {
  swm::util::ReceiverSlice slice;
  slice.size = static_cast<uint32_t>(s.size());
  slice.data.reset(new char[slice.size]);
  if (slice.size > 0) {
    std::memcpy(slice.data.get(), s.data(), slice.size);
  }
  return slice;
}

std::vector<swm::util::ReceiverSlice> seven_nil_slices() {
  std::vector<swm::util::ReceiverSlice> data(swm::util::DataTypeCount);
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] = slice_from_string(etf_version_nil());
  }
  return data;
}

}  // namespace

TEST_F(ctrl, schedule_init_rejects_oversized_jobs_list) {
  auto data = seven_nil_slices();
  const uint32_t oversized = static_cast<uint32_t>(swm::util::kMaxEiListElements) + 1u;
  data[swm::util::SWM_DATA_TYPE_JOBS] = slice_from_string(etf_version_list_header(oversized));

  auto ctx = std::make_shared<swm::util::CommandContext>("1");
  ScheduleCommandTestAccess cmd(ctx);
  std::stringstream errors;
  EXPECT_FALSE(cmd.init(data, &errors));
  EXPECT_FALSE(errors.str().empty());
}

TEST_F(ctrl, schedule_init_rejects_truncated_slice) {
  auto data = seven_nil_slices();
  // Version only -- decode_version may succeed then get_type fails / index OOB.
  data[swm::util::SWM_DATA_TYPE_JOBS] = slice_from_string(std::string("\x83", 1));

  auto ctx = std::make_shared<swm::util::CommandContext>("1");
  ScheduleCommandTestAccess cmd(ctx);
  std::stringstream errors;
  EXPECT_FALSE(cmd.init(data, &errors));
}

TEST(ei_bounds, index_in_bounds) {
  EXPECT_TRUE(swm::util::ei_index_in_bounds(0, 10));
  EXPECT_TRUE(swm::util::ei_index_in_bounds(10, 10));
  EXPECT_FALSE(swm::util::ei_index_in_bounds(11, 10));
  EXPECT_FALSE(swm::util::ei_index_in_bounds(-1, 10));
}
