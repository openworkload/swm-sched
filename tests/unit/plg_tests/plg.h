#pragma once

#include "fcfs_implementation.h"
#include "test_defs.h"

#include <gtest/gtest.h>

#include <algorithm>

class plg : public ::testing::Test {
 public:
  void SetUp() {}
  void TearDown() {}

  swm::PluginEventsInterface *events() { return &events_; }

 private:
  EmptyPluginEvents events_;
};
