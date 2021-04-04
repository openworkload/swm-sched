#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "auxl/my_queue.h"

TEST(auxl, my_queue_logic) {
  swm::util::MyQueue<int> q(4);
  ASSERT_EQ(q.size(), 4);
  ASSERT_EQ(q.element_count(), 0);
  q.push(2);
  q.push(3);
  q.push(4);
  ASSERT_EQ(q.element_count(), 3);
  ASSERT_EQ(q.pop(), 2);
  ASSERT_EQ(q.pop(), 3);
  ASSERT_EQ(q.pop(), 4);
  ASSERT_EQ(q.element_count(), 0);
}

TEST(auxl, my_queue_looping) {
  swm::util::MyQueue<float> q(3);
  q.push(0.0f);
  for (size_t i = 0; i < 10; ++i) {
    q.push(2.0f);
    q.push(3.0f);
    q.pop();
    q.pop();
  }
  ASSERT_EQ(q.element_count(), 1);
  ASSERT_EQ(q.pop(), 3.0f);
}

TEST(auxl, my_queue_picking) {
  int tmp = -1;
  swm::util::MyQueue<int> queue(3);
  ASSERT_ANY_THROW(queue.try_peek(nullptr));
  ASSERT_FALSE(queue.try_peek(&tmp));
  queue.push(2);
  queue.push(3);
  ASSERT_TRUE(queue.try_peek(&tmp));
  ASSERT_TRUE(queue.try_peek(&tmp));
  ASSERT_EQ(tmp, 2);
}

TEST(auxl, my_queue_concurrency) {
  int n = 16 * 1024;
  swm::util::MyQueue<int> queue(1024);
  std::set<int> values;
  bool go_flag = false;
  
  auto writer = [go = &go_flag, q = &queue, n](bool odd) -> void {
    while (!*go) std::this_thread::yield();
    for (int i = (odd ? 1 : 0); i < n; i += 2)
      q->push(i);
  };
  auto reader = [go = &go_flag, q = &queue, vals = &values, n]() -> void {
    while (!*go) std::this_thread::yield();
    for (int i = 0; i < n; ++i)
      vals->emplace(q->pop());
  };
  std::thread writer_thread1(writer, true);
  std::thread writer_thread2(writer, false);
  std::thread reader_thread(reader);

  go_flag = true;
  writer_thread1.join();
  writer_thread2.join();
  reader_thread.join();
  for (int i = 0; i < n; ++i)
    ASSERT_TRUE(values.find(i) != values.end());
}
