#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "ctrl.h"
#include "ctrl/scheduling_info.h"

TEST_F(ctrl, scheduling_info_clusters) {
  swm::util::SchedulingInfo sched;
  sched.clusters_vector().resize(2);
  sched.clusters_vector()[0].set_id("1");
  sched.clusters_vector()[1].set_id("2");
  sched.validate_references();
  ASSERT_EQ(sched.clusters().size(), 2);
  ASSERT_EQ(sched.clusters()[0]->get_id(), "1");
  ASSERT_EQ(sched.clusters()[1]->get_id(), "2");
}

TEST_F(ctrl, scheduling_info_not_valid) {
  swm::util::SchedulingInfo sched;
  sched.clusters_vector().resize(1);
  sched.clusters_vector()[0].set_id("1");
  ASSERT_ANY_THROW(sched.clusters());
  sched.validate_references();
  ASSERT_NO_THROW(sched.clusters());
}
