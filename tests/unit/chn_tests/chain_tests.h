
#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "scheduling_info_presets.h"
#include "chn.h"
#include "chn/chain.h"


TEST_F(chn, chain_wrong_init) {
  std::vector<std::shared_ptr<swm::Algorithm> > alg;
  ASSERT_TRUE(create_fcfs_algorithms(&alg, 1));

  {
    swm::Chain chain;
    std::shared_ptr<swm::SchedulingInfoInterface> res;
    ASSERT_ANY_THROW(chain.init(res, alg));
  }

  {
    swm::Chain chain;
    alg.clear();
    ASSERT_ANY_THROW(chain.init(SchedulingInfoPresets::one_node_one_job("1"), alg));
  }
}

TEST_F(chn, chain_no_init) {
  swm::Chain chain;
  ASSERT_EQ(chain.status(), swm::Chain::NOT_STARTED);
  ASSERT_EQ(chain.stopped(), true);
  
  const std::shared_ptr<swm::TimetableInfoInterface> tt;
  ASSERT_ANY_THROW(chain.ready_for_async_operation());
  ASSERT_ANY_THROW(chain.interrupt_async());
  ASSERT_ANY_THROW(chain.inject_timetable_async(tt));
  ASSERT_ANY_THROW(chain.actual_timetable());
  ASSERT_ANY_THROW(chain.intermediate_timetable());
  ASSERT_ANY_THROW(chain.algorithms());
  ASSERT_ANY_THROW(chain.metrics());
}

TEST_F(chn, chain_multiple_init) {
  std::vector<std::shared_ptr<swm::Algorithm> > alg;
  ASSERT_TRUE(create_fcfs_algorithms(&alg, 1));
  swm::Chain chain;
  ASSERT_NO_THROW(chain.init(SchedulingInfoPresets::one_node_one_job("1"), alg));
  ASSERT_ANY_THROW(chain.init(SchedulingInfoPresets::one_node_one_job("1"), alg));
}

TEST_F(chn, chain_simple_tt) {
  std::vector<std::shared_ptr<swm::Algorithm> > alg;
  ASSERT_TRUE(create_fcfs_algorithms(&alg, 1));

  swm::Chain chain;
  ASSERT_NO_THROW(chain.init(SchedulingInfoPresets::one_node_one_job("1"), alg));
  while (!chain.stopped()) { std::this_thread::yield(); }

  ASSERT_EQ(chain.status(), swm::Chain::FINISHED);
  ASSERT_EQ(chain.intermediate_timetable().get(), nullptr);
  ASSERT_NE(chain.actual_timetable().get(), nullptr);

  ASSERT_FALSE(chain.actual_timetable()->empty());
}

TEST_F(chn, chain_interrupt_async) {
  std::vector<std::shared_ptr<swm::Algorithm> > alg;
  ASSERT_TRUE(create_dummy_algorithms(&alg, 1));

  swm::Chain chain;
  ASSERT_NO_THROW(chain.init(SchedulingInfoPresets::one_node_one_job("hold_on"), alg));
  while (chain.intermediate_timetable().get() == nullptr) { std::this_thread::yield(); }
  ASSERT_FALSE(chain.intermediate_timetable()->empty());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  ASSERT_EQ(chain.status(), swm::Chain::WORKING);
  ASSERT_TRUE(chain.ready_for_async_operation());
  ASSERT_NO_THROW(chain.interrupt_async());
  while (!chain.stopped()) { std::this_thread::yield(); }
  ASSERT_EQ(chain.status(), swm::Chain::INTERRUPTED);
}

TEST_F(chn, chain_inject_timetable_async) {
  std::vector<std::shared_ptr<swm::Algorithm> > alg;
  ASSERT_TRUE(create_fcfs_algorithms(&alg, 1));
  swm::Chain fcfs_chain;
  ASSERT_NO_THROW((fcfs_chain.init(SchedulingInfoPresets::one_node_one_job("1"), alg)));
  while (!fcfs_chain.stopped()) { std::this_thread::yield(); }
  ASSERT_EQ(fcfs_chain.status(), swm::Chain::FINISHED);
  ASSERT_NE(fcfs_chain.actual_timetable().get(), nullptr);
  ASSERT_FALSE(fcfs_chain.actual_timetable()->empty());

  ASSERT_TRUE(create_dummy_algorithms(&alg, 1));
  swm::Chain dummy_chain;
  ASSERT_NO_THROW((dummy_chain.init(SchedulingInfoPresets::one_node_one_job("hold_on"),
                                    alg)));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  ASSERT_EQ(dummy_chain.status(), swm::Chain::WORKING);
  ASSERT_NE(dummy_chain.intermediate_timetable().get(), nullptr);
  ASSERT_FALSE(dummy_chain.intermediate_timetable()->tables().empty());
  ASSERT_EQ(dummy_chain.intermediate_timetable()->tables()[0]->get_job_id(), "hold_on");
  
  ASSERT_TRUE(dummy_chain.ready_for_async_operation());
  ASSERT_NO_THROW(dummy_chain.inject_timetable_async(fcfs_chain.actual_timetable()));
  while (!dummy_chain.stopped()) { std::this_thread::yield(); }
  ASSERT_EQ(dummy_chain.status(), swm::Chain::FINISHED);
  ASSERT_NE(dummy_chain.actual_timetable().get(), nullptr);
  ASSERT_FALSE(dummy_chain.actual_timetable()->empty());
  ASSERT_FALSE(dummy_chain.actual_timetable()->tables().empty());
  ASSERT_EQ(dummy_chain.actual_timetable()->tables()[0]->get_job_id(), "1");
}

TEST_F(chn, chain_with_three_algs) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  {
    ASSERT_TRUE(create_fcfs_algorithms(&algs, 3));
    swm::Chain fcfs_chain;
    ASSERT_NO_THROW(fcfs_chain.init(SchedulingInfoPresets::one_node_one_job("1"), algs));
    while (!fcfs_chain.stopped()) { std::this_thread::yield(); }
    ASSERT_EQ(fcfs_chain.status(), swm::Chain::FINISHED);
  }

  {
    ASSERT_TRUE(create_dummy_algorithms(&algs, 3));
    swm::Chain dummy_chain;
    ASSERT_NO_THROW(dummy_chain.init(SchedulingInfoPresets::one_node_one_job("hold_on"),
                                     algs));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_EQ(dummy_chain.status(), swm::Chain::WORKING);
  } // chain must be stopped by destructor
}

TEST_F(chn, chain_time_counting) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_dummy_algorithms(&algs, 1));
  std::shared_ptr<swm::util::TimeCounter> counter(new swm::util::TimeCounter());
  {
    swm::Chain dummy_chain;
    counter->reset();
    ASSERT_NO_THROW(dummy_chain.init(SchedulingInfoPresets::one_node_one_job("hold_on"),
                                     algs, counter));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  double working = 0.0;
  counter->get_times(nullptr, nullptr, &working);
  ASSERT_LE(working, 0.06);
  ASSERT_GE(working, 0.04);
}
