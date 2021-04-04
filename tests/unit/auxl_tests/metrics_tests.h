
#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "auxl/metrics.h"

TEST(auxl, metrics_register_get_set) {
  swm::util::Metrics metrics;
  ASSERT_NO_THROW(metrics.register_double_value(32, "double"));
  ASSERT_NO_THROW(metrics.update_double_value(32, 2.0));
  ASSERT_EQ(metrics.double_value(32), 2.0);

  ASSERT_NO_THROW(metrics.register_int_value(0, "int"));
  ASSERT_EQ(metrics.update_int_value(0, 4), 4);
  ASSERT_EQ(metrics.int_value(0), 4);
}

TEST(auxl, metrics_update_reset) {
  swm::util::Metrics metrics;
  ASSERT_NO_THROW(metrics.register_int_value(1, "my value"));
  ASSERT_EQ(metrics.int_value(1), 0);
  ASSERT_EQ(metrics.update_int_value(1, 10), 10);
  ASSERT_EQ(metrics.update_int_value(1, 20), 30);
  ASSERT_NO_THROW(metrics.reset_int_value(1));
  ASSERT_EQ(metrics.int_value(1), 0);
}

TEST(auxl, metrics_wrong_register) {
  swm::util::Metrics metrics;
  ASSERT_NO_THROW(metrics.register_int_value(0, "test"));
  ASSERT_ANY_THROW(metrics.register_int_value(0, "new value"));
  ASSERT_ANY_THROW(metrics.double_value(0));
  ASSERT_ANY_THROW(metrics.add_double_value_handler(0, [](double, double) -> void { }));
}

TEST(auxl, metrics_enumeration) {
  swm::util::Metrics metrics;
  ASSERT_NO_THROW(metrics.register_int_value(2, "1st_value"));
  ASSERT_NO_THROW(metrics.register_int_value(3, "2nd_value"));
  ASSERT_NO_THROW(metrics.register_int_value(5, "3rd_value"));
  std::vector<std::pair<uint32_t, std::string> > en;
  ASSERT_NO_THROW(en = metrics.int_value_indices());
  ASSERT_EQ(en.size(), 3);
  ASSERT_EQ(en[0].first, 2);
  ASSERT_EQ(en[0].second, "1st_value");
  ASSERT_EQ(en[1].first, 3);
  ASSERT_EQ(en[1].second, "2nd_value");
  ASSERT_EQ(en[2].first, 5);
  ASSERT_EQ(en[2].second, "3rd_value");

  ASSERT_NO_THROW(metrics.register_int_value(7, "4th_value"));
  ASSERT_NO_THROW(en = metrics.int_value_indices());
  ASSERT_EQ(en.size(), 4);
  ASSERT_EQ(en[3].first, 7);
  ASSERT_EQ(en[3].second, "4th_value");
}

TEST(auxl, metrics_events) {
  swm::util::Metrics metrics;
  int old_value = 0, new_value = 0;
  auto handler = [ov = &old_value, nv = &new_value](int old_value, int new_value) -> void {
    *ov = old_value;
    *nv = new_value;
  };
  ASSERT_NO_THROW(metrics.register_int_value(0, "test value"));
  ASSERT_NO_THROW(metrics.add_int_value_handler(0, handler));

  ASSERT_NO_THROW(metrics.update_int_value(0, 10));
  ASSERT_EQ(old_value, 0);
  ASSERT_EQ(new_value, 10);

  ASSERT_NO_THROW(metrics.reset_int_value(0));
  ASSERT_EQ(old_value, 10);
  ASSERT_EQ(new_value, 0);
}

TEST(auxl, metrics_concurrency) {
  swm::util::Metrics metrics;
  ASSERT_NO_THROW(metrics.register_int_value(0, "just a record"));
  
  bool go = false;
  std::atomic<int> prep_threads(0);
  auto func = [m = &metrics, go_flag = &go, counter = &prep_threads]() -> void {
    counter->fetch_add(1);
    while (*go_flag) { std::this_thread::yield(); }
    for (int i = 0; i < 10000; ++i) {
      m->update_int_value(0, 1);
    }
  };

  std::thread t1(func);
  std::thread t2(func);
  while (prep_threads.load() != 2) { std::this_thread::yield(); }
  go = true;
  if (t1.joinable()) t1.join();
  if (t2.joinable()) t2.join();
  ASSERT_EQ(metrics.int_value(0), 20000);
}

TEST(auxl, metrics_cloning) {
  swm::util::Metrics m;
  ASSERT_NO_THROW(m.register_int_value(1, "i#1"));
  ASSERT_NO_THROW(m.update_int_value(1, 2));
  ASSERT_NO_THROW(m.register_double_value(2, "d#2"));
  ASSERT_NO_THROW(m.update_double_value(2, 3.0));

  int extracted_value = 0;
  ASSERT_NO_THROW(m.add_int_value_handler(1, [ev = &extracted_value](int, int new_v) -> void{
    *ev = new_v;
  }));

  std::shared_ptr<swm::MetricsInterface> cloned;
  ASSERT_NO_THROW(cloned = m.clone());
  ASSERT_EQ(cloned->int_value(1), 2);
  ASSERT_EQ(cloned->double_value(2), 3.0);
  ASSERT_EQ(cloned->update_int_value(1, 4), 6);
  ASSERT_EQ(extracted_value, 6);
}