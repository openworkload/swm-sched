#pragma once

#include <gtest/gtest.h>
#include <algorithm>

#include "test_defs.h"
#include "fcfs_implementation.h"


class plg : public ::testing::Test {
 public:
  void SetUp() { }
  void TearDown() { }

  swm::PluginEventsInterface *events() { return &events_; }

 private:
  EmptyPluginEvents events_;
};
