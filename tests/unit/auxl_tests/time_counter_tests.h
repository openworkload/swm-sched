#pragma once

#include <gtest/gtest.h>
#include <atomic>

#include "test_defs.h"
#include "auxl/time_counter.h"

// Feel free to adjust this values
const int SLEEP_MS = 50;
const int ERROR_MS = 25;

TEST(auxl, time_counter_working) {
  swm::util::TimeCounter counter;
  ASSERT_NO_THROW(counter.turn_on());
  std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
  ASSERT_NO_THROW(counter.turn_off());
  double astro = 0.0, idling = 0.0, working = 0.0;
  ASSERT_NO_THROW(counter.get_times(&astro, &idling, &working));
  ASSERT_LT(idling, ERROR_MS * 1e-3);
  ASSERT_GT(astro, (SLEEP_MS - ERROR_MS) * 1e-3);
  ASSERT_LT(astro, (SLEEP_MS + ERROR_MS) * 1e-3);
  ASSERT_GT(working, (SLEEP_MS - ERROR_MS) * 1e-3);
  ASSERT_LT(working, (SLEEP_MS + ERROR_MS) * 1e-3);
}

TEST(auxl, time_counter_idling) {
  swm::util::TimeCounter counter;
  std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
  double astro = 0.0, idling = 0.0, working = 0.0;
  ASSERT_NO_THROW(counter.get_times(&astro, &idling, &working));
  ASSERT_LT(working, ERROR_MS * 1e-3);
  ASSERT_GT(astro, (SLEEP_MS - ERROR_MS) * 1e-3);
  ASSERT_LT(astro, (SLEEP_MS + ERROR_MS) * 1e-3);
  ASSERT_GT(idling, (SLEEP_MS - ERROR_MS) * 1e-3);
  ASSERT_LT(idling, (SLEEP_MS + ERROR_MS) * 1e-3);
}

TEST(auxl, time_counter_nullptrs) {
  swm::util::TimeCounter counter;
  counter.turn_on();
  std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
  counter.turn_off();
  std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
  double astro = 0.0, idling = 0.0, working = 0.0;
  ASSERT_NO_THROW(counter.get_times(&astro, nullptr, nullptr));
  ASSERT_GT(astro, (SLEEP_MS - ERROR_MS) * 2e-3);
  ASSERT_LT(astro, (SLEEP_MS + ERROR_MS) * 2e-3);
  ASSERT_NO_THROW(counter.get_times(nullptr, &idling, nullptr));
  ASSERT_GT(idling, (SLEEP_MS - ERROR_MS) * 1e-3);
  ASSERT_LT(idling, (SLEEP_MS + ERROR_MS) * 1e-3);
  ASSERT_NO_THROW(counter.get_times(nullptr, nullptr, &working));
  ASSERT_GT(working, (SLEEP_MS - ERROR_MS) * 1e-3);
  ASSERT_LT(working, (SLEEP_MS + ERROR_MS) * 1e-3);
  ASSERT_NO_THROW(counter.get_times(nullptr, nullptr, nullptr));
}

TEST(auxl, time_counter_multiple_calls) {
  swm::util::TimeCounter counter;
  for (int i = 0; i < 10; i++) {
    ASSERT_NO_THROW(counter.turn_on());
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_NO_THROW(counter.turn_off());
  }
  double working = 0.0;
  ASSERT_NO_THROW(counter.get_times(nullptr, nullptr, &working));
  ASSERT_GT(working, (SLEEP_MS - ERROR_MS) * 10e-3);
  ASSERT_LT(working, (SLEEP_MS + ERROR_MS) * 10e-3);
}

TEST(auxl, time_counter_off_from_another_thread) {
  swm::util::TimeCounter counter;
  ASSERT_NO_THROW(counter.turn_on());
  volatile bool thrown = false;
  auto thread_func = [cnt = &counter, flag = &thrown]() -> void {
    try { cnt->turn_off(); *flag = false; }
    catch (const std::runtime_error &) { *flag = true; }
  };
  std::thread thread = std::thread(thread_func);
  if (thread.joinable()) thread.join();
  ASSERT_TRUE(thrown);
}

TEST(auxl, time_counter_no_start) {
  swm::util::TimeCounter counter;
  ASSERT_ANY_THROW(counter.turn_off());
}

TEST(auxl, time_counter_double_stop) {
  swm::util::TimeCounter counter;
  ASSERT_NO_THROW(counter.turn_on());
  ASSERT_NO_THROW(counter.turn_off());
  ASSERT_ANY_THROW(counter.turn_off());
}

TEST(auxl, time_counter_correct_reset) {
  swm::util::TimeCounter counter;
  ASSERT_NO_THROW(counter.turn_on());
  std::this_thread::sleep_for(std::chrono::milliseconds(2 * SLEEP_MS));
  ASSERT_NO_THROW(counter.turn_off());
  ASSERT_NO_THROW(counter.reset());
  ASSERT_NO_THROW(counter.turn_on());
  std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
  ASSERT_NO_THROW(counter.turn_off());
  double astro = 0.0;
  ASSERT_NO_THROW(counter.get_times(&astro, nullptr, nullptr));
  ASSERT_GT(astro, (SLEEP_MS - ERROR_MS) * 1e-3);
  ASSERT_LT(astro, (SLEEP_MS + ERROR_MS) * 1e-3);
}

TEST(auxl, time_counter_wrong_reset) {
  swm::util::TimeCounter counter;
  ASSERT_NO_THROW(counter.turn_on());
  ASSERT_ANY_THROW(counter.reset());
  ASSERT_NO_THROW(counter.turn_off());
}

TEST(auxl, time_counter_concurrency) {
  swm::util::TimeCounter counter;
  std::atomic<int> threads_cnt(0);
  volatile bool go = false;
  auto thread_func = [cnt = &counter, threads = &threads_cnt, go = &go]() -> void {
    threads->fetch_add(1);
    while (!*go) { std::this_thread::yield(); }
    cnt->turn_on();
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    cnt->turn_off();
  };

  std::vector<std::thread> threads(10);
  for (auto & t : threads) { t = std::thread(thread_func); }
  while (threads_cnt.load() != (int)threads.size()) { std::this_thread::yield(); }
  counter.reset();
  std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
  go = true;
  for (auto & t : threads) { if (t.joinable()) t.join(); }
  
  double astro = 0.0, idling = 0.0, working = 0.0;
  counter.get_times(&astro, &idling, &working);
  ASSERT_GT(astro, (SLEEP_MS - ERROR_MS) * 2e-3);
  ASSERT_GT(idling, (SLEEP_MS - ERROR_MS) * 1e-3);
  ASSERT_GT(working, (SLEEP_MS - ERROR_MS) * 10e-3);
}

TEST(auxl, time_counter_via_locker) {
  std::shared_ptr<swm::util::TimeCounter> counter(new swm::util::TimeCounter());
  {
    swm::util::TimeCounter::Lock lock(counter);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
  }
  double astro = 0.0, working = 0.0;
  ASSERT_NO_THROW(counter->get_times(&astro, nullptr, &working));
  ASSERT_GT(astro, (SLEEP_MS - ERROR_MS) * 1e-3);
  ASSERT_LT(astro, (SLEEP_MS + ERROR_MS) * 1e-3);
  ASSERT_GT(working, (SLEEP_MS - ERROR_MS) * 1e-3);
  ASSERT_LT(working, (SLEEP_MS + ERROR_MS) * 1e-3);
}
