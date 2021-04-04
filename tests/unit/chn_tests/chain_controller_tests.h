
#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "chn.h"
#include "chn/chain_controller.h"

TEST_F(chn, chain_controller_no_init) {
  swm::util::ChainController ctrler;
  ASSERT_ANY_THROW(ctrler.finished());
  ASSERT_ANY_THROW(ctrler.invoke_interrupt(empty_finish_callback()));
  ASSERT_ANY_THROW(ctrler.invoke_stats([]
      (bool,
       const std::shared_ptr<swm::util::MetricsSnapshot> &) -> void { }
      ));
}

TEST_F(chn, chain_controller_multiple_init) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_fcfs_algorithms(&algs, 1));
  std::shared_ptr<swm::Chain> chain(new swm::Chain());
  ASSERT_NO_THROW(chain->init(SchedulingInfoPresets::one_node_one_job("1"), algs));
  swm::util::Metrics metrics;
  
  {
    swm::util::ChainController ctrler;
    ASSERT_NO_THROW(ctrler.init(chain, &metrics, empty_finish_callback(), 1.0));
    ASSERT_ANY_THROW(ctrler.init(chain, &metrics, empty_finish_callback(), 1.0));
  }
}

TEST_F(chn, chain_controller_wrong_init) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_fcfs_algorithms(&algs, 1));
  std::shared_ptr<swm::Chain> chain(new swm::Chain());
  ASSERT_NO_THROW(chain->init(SchedulingInfoPresets::one_node_one_job("1"), algs));
  swm::util::Metrics metrics;

  {
    swm::util::ChainController ctrler;
    std::shared_ptr<swm::Chain> null_chain;
    ASSERT_ANY_THROW(ctrler.init(null_chain, &metrics, empty_finish_callback(), 1.0));
    ASSERT_ANY_THROW(ctrler.init(chain, nullptr, empty_finish_callback(), 1.0));
    ASSERT_ANY_THROW(ctrler.init(chain, nullptr, empty_finish_callback(), 0.0));
  }
}

TEST_F(chn, chain_controller_finish_clb) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_fcfs_algorithms(&algs, 3));
  std::shared_ptr<swm::Chain> chain(new swm::Chain());
  ASSERT_NO_THROW(chain->init(SchedulingInfoPresets::one_node_one_job("1"), algs));
  swm::util::Metrics metrics;
  
  {
    swm::util::ChainController ctrler;
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    auto clb = [_tt = &tt](bool,
                           const std::shared_ptr<swm::TimetableInfoInterface> &tt,
                           const std::shared_ptr<swm::util::MetricsSnapshot> &) -> void {
      *_tt = tt;
    };
    ASSERT_NO_THROW(ctrler.init(chain, &metrics, clb, 1.0));
    while (!ctrler.finished()) { std::this_thread::yield(); }
    ASSERT_NE(tt.get(), nullptr);
    ASSERT_EQ(tt->tables().size(), 1);
    ASSERT_EQ(tt->tables()[0]->get_job_id(), "1");
  }
}

TEST_F(chn, chain_controller_interrupt) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_dummy_algorithms(&algs, 1));
  std::shared_ptr<swm::Chain> chain(new swm::Chain());
  ASSERT_NO_THROW(chain->init(SchedulingInfoPresets::one_node_one_job("hold_on"), algs));
  swm::util::Metrics metrics;

  {
    volatile bool tt_scheduled = true;             // should be switched to false
    volatile bool chain_interrupted = false;       // should be switched to true
    swm::util::ChainController ctrler;
    ASSERT_NO_THROW(ctrler.init(chain, &metrics, empty_finish_callback(&tt_scheduled), 10.0));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_FALSE(ctrler.finished());
    ASSERT_NO_THROW(ctrler.invoke_interrupt(empty_finish_callback(&chain_interrupted)));
    while (!ctrler.finished()) { std::this_thread::yield(); }
    ASSERT_TRUE(chain_interrupted);
    ASSERT_FALSE(tt_scheduled);
  }
}

TEST_F(chn, chain_controller_exchange) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs1, algs2;
  std::vector<std::shared_ptr<swm::Algorithm> > fcfs_alg, dummy_alg;
  ASSERT_TRUE(create_fcfs_algorithms(&fcfs_alg, 1));
  ASSERT_TRUE(create_dummy_algorithms(&dummy_alg, 1));
  algs1.push_back(fcfs_alg[0]);
  algs1.push_back(dummy_alg[0]);
  ASSERT_TRUE(create_fcfs_algorithms(&fcfs_alg, 1));
  ASSERT_TRUE(create_dummy_algorithms(&dummy_alg, 1));
  algs2.push_back(fcfs_alg[0]);
  algs2.push_back(dummy_alg[0]);

  std::shared_ptr<swm::Chain> chain1(new swm::Chain()), chain2(new swm::Chain());
  ASSERT_NO_THROW(chain1->init(SchedulingInfoPresets::one_node_one_job("hold_on"), algs1));
  ASSERT_NO_THROW(chain2->init(SchedulingInfoPresets::one_node_one_job("hold_on"), algs2));
  swm::util::Metrics metrics;
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  {
    swm::util::ChainController ctrler1, ctrler2;
    ASSERT_NO_THROW(ctrler1.init(chain1, &metrics, empty_finish_callback(), 10.0));
    ASSERT_NO_THROW(ctrler2.init(chain2, &metrics, empty_finish_callback(), 10.0));
    bool exch1 = false, exch2 = false;
    ASSERT_NO_THROW(ctrler1.invoke_exchange(&ctrler2, [flag = &exch1](bool success) -> void {
      *flag = success;
    }));
    ASSERT_NO_THROW(ctrler2.invoke_exchange(&ctrler1, [flag = &exch2](bool success) -> void {
      *flag = success;
    }));
    ASSERT_NO_THROW(ctrler1.invoke_interrupt(empty_finish_callback()));
    while (!ctrler1.finished()) { std::this_thread::yield(); }
    ASSERT_TRUE(exch1);
    ASSERT_TRUE(exch2);
  }

}

TEST_F(chn, chain_controller_exchange_timeout) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs1, algs2;
  ASSERT_TRUE(create_dummy_algorithms(&algs1, 1));
  ASSERT_TRUE(create_dummy_algorithms(&algs2, 1));
  std::shared_ptr<swm::Chain> chain1(new swm::Chain()), chain2(new swm::Chain());
  ASSERT_NO_THROW(chain1->init(SchedulingInfoPresets::one_node_one_job("hold_on"), algs1));
  ASSERT_NO_THROW(chain2->init(SchedulingInfoPresets::one_node_one_job("hold_on"), algs2));
  swm::util::Metrics metrics;

  {
    volatile bool success = true;                   // should be switched to false
    swm::util::ChainController ctrler1, ctrler2;
    ASSERT_NO_THROW(ctrler1.init(chain1, &metrics, empty_finish_callback(), 0.2));
    ASSERT_NO_THROW(ctrler2.init(chain2, &metrics, empty_finish_callback(), 0.2));

    auto t_start = std::chrono::steady_clock::now();
    ASSERT_NO_THROW(ctrler1.invoke_exchange(&ctrler2, [flag = &success](bool success) -> void {
      *flag = success;
    }));
    double seconds = 0.0;
    do {
      std::this_thread::yield();
      seconds = (double)std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::steady_clock::now() - t_start).count() * 1e-6;
    }
    while (success && seconds < 0.5);
    ASSERT_LT(seconds, 0.25);
    ASSERT_GT(seconds, 0.15);
    ASSERT_FALSE(success);
  }
}

TEST_F(chn, chain_controller_metrics) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_dummy_algorithms(&algs, 1));
  std::shared_ptr<swm::Chain> chain(new swm::Chain());
  ASSERT_NO_THROW(chain->init(SchedulingInfoPresets::one_node_one_job("hold_on"), algs));
  
  swm::util::Metrics metrics;
  ASSERT_NO_THROW(metrics.register_int_value(1, "#1"));
  ASSERT_EQ(metrics.update_int_value(1, 10), 10);

  {
    swm::util::ChainController ctrler;
    ASSERT_NO_THROW(ctrler.init(chain, &metrics, empty_finish_callback(), 10.0));
    bool success = false;           // should be switched to true
    std::shared_ptr<swm::util::MetricsSnapshot> snapshot;
    ASSERT_NO_THROW(ctrler.invoke_stats([flag = &success, ss = &snapshot]
                                        (bool success,
                                         const std::shared_ptr<swm::util::MetricsSnapshot> &m)
                                        -> void {
      *flag = success;
      *ss = m;
    }));
    ASSERT_NO_THROW(ctrler.invoke_interrupt(empty_finish_callback()));
    while (!ctrler.finished()) { std::this_thread::yield(); }
    ASSERT_TRUE(success);
    ASSERT_NE(snapshot.get(), nullptr);
    auto indices = snapshot->service_metrics().int_value_indices();
    ASSERT_EQ(indices.size(), 1);
    ASSERT_EQ(indices[0].first, 1);
    ASSERT_EQ(indices[0].second, "#1");
    ASSERT_EQ(snapshot->service_metrics().int_value(1), 10);
  }
}

TEST_F(chn, chain_controller_time_counting) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_dummy_algorithms(&algs, 1));
  std::shared_ptr<swm::Chain> chain(new swm::Chain());
  ASSERT_NO_THROW(chain->init(SchedulingInfoPresets::one_node_one_job("hold_on"), algs));
  
  swm::util::Metrics metrics;
  std::shared_ptr<swm::util::TimeCounter> timer(new swm::util::TimeCounter());
  {
    swm::util::ChainController ctrler;
    ASSERT_NO_THROW(ctrler.init(chain, &metrics, empty_finish_callback(), 10.0, timer));
    ASSERT_NO_THROW(ctrler.invoke_interrupt(empty_finish_callback()));
  }
  double working;
  timer->get_times(nullptr, nullptr, &working);
  ASSERT_NE(working, 0.0);    // at least, some overheads must be measured
}
