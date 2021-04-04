
#pragma once

#include <iostream>
#include <gtest/gtest.h>

#include "test_defs.h"
#include "ctrl.h"
#include "ctrl/sender.h"
#include "ctrl/responses.h"
#include "chn/metrics_snapshot.h"


TEST_F(ctrl, sender_start_stop) {
  std::unique_ptr<swm::util::Sender> ptr(new swm::util::Sender());
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > queue(2);
  ASSERT_NO_THROW(ptr->init(&queue, &std::cout));
  ptr.reset();
}

TEST_F(ctrl, sender_nullptrs) {
  swm::util::Sender sender;
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > queue(2);
  ASSERT_ANY_THROW(sender.init(&queue, nullptr));
  ASSERT_ANY_THROW(sender.init(nullptr, &std::cout));
}

TEST_F(ctrl, sender_second_init) {
  swm::util::Sender sender;
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > queue(2);
  ASSERT_NO_THROW(sender.init(&queue, &std::cout));
  ASSERT_ANY_THROW(sender.init(&queue, &std::cout));
}

TEST_F(ctrl, sender_no_init) {
  swm::util::Sender sender;
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > queue(2);
  ASSERT_ANY_THROW(sender.close());
}

TEST_F(ctrl, sender_nullptr_as_response) {
  swm::util::Sender sender;
  std::stringstream oss;
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > queue(2);
  queue.push(std::shared_ptr<swm::util::ResponseInterface>());
  queue.push(std::shared_ptr<swm::util::ResponseInterface>());
  ASSERT_NO_THROW(sender.init(&queue, &oss));
  ASSERT_NO_THROW(sender.close());
  ASSERT_EQ(queue.element_count(), 0);
  ASSERT_EQ(oss.str().size(), 0);
}

TEST_F(ctrl, sender_failed_response) {
  swm::util::Sender sender;
  std::stringstream oss;
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > queue(2);
  ASSERT_NO_THROW(sender.init(&queue, &oss));
  
  std::shared_ptr<swm::util::CommandContext> ctx(new swm::util::CommandContext("0"));
  std::shared_ptr<swm::util::ResponseInterface> resp(new swm::util::EmptyResponse(ctx, false));
  queue.push(resp);
  
  ASSERT_NO_THROW(sender.close());
  ASSERT_EQ(queue.element_count(), 0);
  ASSERT_GE(oss.str().size(), 0);
}

TEST_F(ctrl, sender_timetable_response) {
  swm::util::Sender sender;
  std::stringstream oss;
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > queue(2);
  ASSERT_NO_THROW(sender.init(&queue, &oss));
  
  swm::SwmTimetable table1, table2;
  table1.set_job_id("1");
  std::vector<std::string> nodes1; nodes1.push_back("4");
  table1.set_job_nodes(nodes1);
  table1.set_start_time(0);
  table2.set_job_id("2");
  std::vector<std::string> nodes2; nodes2.push_back("3");
  table2.set_job_nodes(nodes2);
  table2.set_start_time(100);

  std::shared_ptr<swm::util::CommandContext> ctx(new swm::util::CommandContext("1"));
  std::vector<const swm::SwmTimetable *> tables;
  tables.push_back(&table1); tables.push_back(&table2);
  std::shared_ptr<swm::TimetableInfoInterface> tt(new TimetableInfoForTests(tables));
  std::shared_ptr<swm::util::MetricsSnapshot> m(new swm::util::MetricsSnapshot());
  std::shared_ptr<swm::util::ResponseInterface> resp(new swm::util::TimetableResponse(ctx, tt, m));
  queue.push(resp);

  ASSERT_NO_THROW(sender.close());
  ASSERT_EQ(queue.element_count(), 0);
  ASSERT_GE(oss.str().size(), 20);
}

TEST_F(ctrl, sender_multiple_responses) {
  swm::util::Sender sender;
  std::stringstream oss;
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > queue(2);
  ASSERT_NO_THROW(sender.init(&queue, &oss));

  std::shared_ptr<swm::util::CommandContext> ctx1(new swm::util::CommandContext("1"));
  std::shared_ptr<swm::util::ResponseInterface> resp1(new swm::util::EmptyResponse(ctx1, false));
  queue.push(resp1);

  std::shared_ptr<swm::util::CommandContext> ctx2(new swm::util::CommandContext("2"));
  swm::SwmTimetable empty_table;
  std::shared_ptr<swm::TimetableInfoInterface> table2(new TimetableInfoForTests(&empty_table));
  std::shared_ptr<swm::util::MetricsSnapshot> m(new swm::util::MetricsSnapshot());
  std::shared_ptr<swm::util::ResponseInterface> resp2(new swm::util::TimetableResponse(ctx2,
                                                                                       table2, m));
  queue.push(resp2);
  
  ASSERT_NO_THROW(sender.close());
  ASSERT_EQ(queue.element_count(), 0);
  ASSERT_GE(oss.str().size(), 9);
}

TEST_F(ctrl, sender_multiple_start_stop) {
  const int n = 10;
  swm::util::Sender sender;
  std::stringstream oss;
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > queue(2);
  for (int i = 0; i < n; ++i) {
    ASSERT_NO_THROW(sender.init(&queue, &oss));
    std::shared_ptr<swm::util::CommandContext> ctx(new swm::util::CommandContext("0"));
    std::shared_ptr<swm::util::ResponseInterface> resp(new swm::util::EmptyResponse(ctx, true));
    queue.push(resp);
    ASSERT_NO_THROW(sender.close());
    ASSERT_EQ(queue.element_count(), 0);
  }
  ASSERT_GE(oss.str().size(), 5 * n);
}
