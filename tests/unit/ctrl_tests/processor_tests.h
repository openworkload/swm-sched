
#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "scheduling_info_presets.h"
#include "ctrl.h"
#include "ctrl/processor.h"

TEST_F(ctrl, processor_no_init) {
  swm::util::Processor processor;
  ASSERT_ANY_THROW(processor.close());
}

TEST_F(ctrl, processor_multiple_init) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > in_queue(1);
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > out_queue(1);
  {
    swm::util::Processor processor;
    ASSERT_NO_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
    ASSERT_ANY_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
  }
}

TEST_F(ctrl, processor_unknown_algorithm) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > in_queue(1);
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > out_queue(1);
  {
    swm::util::Processor processor;
    ASSERT_NO_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
    auto req = create_schedule_request("#unknown_algorithm", { "swm-unknown" },
                                       SchedulingInfoPresets::one_node_one_job("1"));
    in_queue.push(req);
    ASSERT_NO_THROW(processor.close());
    ASSERT_EQ(out_queue.element_count(), 1);
    auto resp = out_queue.pop();
    ASSERT_EQ(resp->context()->id(), "#unknown_algorithm");
    ASSERT_FALSE(resp->succeeded());
  }
}

TEST_F(ctrl, processor_corrupted_data) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > in_queue(1);
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > out_queue(1);
  {
    swm::util::Processor processor;
    ASSERT_NO_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
    in_queue.push(std::shared_ptr<swm::util::CommandInterface>(
                              new swm::util::CorruptedCommand(create_context("#corrupted_data"))));
    ASSERT_NO_THROW(processor.close());
    ASSERT_EQ(out_queue.element_count(), 1);
    auto resp = out_queue.pop();
    ASSERT_EQ(resp->context()->id(), "#corrupted_data");
    ASSERT_FALSE(resp->succeeded());
  }
}

TEST_F(ctrl, processor_schedule) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > in_queue(1);
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > out_queue(1);
  {
    swm::util::Processor processor;
    ASSERT_NO_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
    in_queue.push(create_schedule_request("#schedule", { "swm-fcfs" },
                                          SchedulingInfoPresets::one_node_one_job("1")));
    ASSERT_NO_THROW(processor.close());

    ASSERT_EQ(out_queue.element_count(), 1);
    auto resp = out_queue.pop();
    ASSERT_TRUE(resp->succeeded());
    ASSERT_EQ(resp->context()->id(), "#schedule");
    // We cannot check timetable, it's a private data
  }
}

TEST_F(ctrl, processor_interrupt) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > in_queue(4);
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > out_queue(4);
  {
    swm::util::Processor processor;
    ASSERT_NO_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
    in_queue.push(create_schedule_request("#schedule1", { "swm-dummy" },
                                          SchedulingInfoPresets::one_node_one_job("hold_on")));
    in_queue.push(create_schedule_request("#schedule2", { "swm-fcfs", "swm-fcfs", "swm-dummy" },
                                          SchedulingInfoPresets::one_node_one_job("hold_on")));

    in_queue.push(std::shared_ptr<swm::util::CommandInterface>(
                                          create_interrupt_request("#interrupt1", "#schedule2")));
    in_queue.push(std::shared_ptr<swm::util::CommandInterface>(
                                          create_interrupt_request("#interrupt2", "#schedule1")));
    
    ASSERT_NO_THROW(processor.close());
    ASSERT_EQ(out_queue.element_count(), 4);
    while (out_queue.element_count() > 0) {
      auto resp = out_queue.pop();
      // Responses are not sorted, so we use if's to differ them
      if (resp->context()->id() == "#schedule1" || resp->context()->id() == "#schedule2") {
        ASSERT_FALSE(resp->succeeded());
      }
      else if (resp->context()->id() == "#interrupt1" || resp->context()->id() == "#interrupt2") {
        ASSERT_TRUE(resp->succeeded());
      }
      else {
        ASSERT_TRUE(false) << "Wrong SwmUID";
      }
    }
  }
}

TEST_F(ctrl, processor_metrics) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > in_queue(3);
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > out_queue(3);
  {
    swm::util::Processor processor;
    ASSERT_NO_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
    in_queue.push(create_schedule_request("#schedule", { "swm-dummy" },
                                          SchedulingInfoPresets::one_node_one_job("hold_on")));
    in_queue.push(create_metrics_request("#metrics", "#schedule"));
    in_queue.push(create_interrupt_request("#interrupt", "#schedule"));
  }
  ASSERT_EQ(out_queue.element_count(), 3);
  while (out_queue.element_count() > 0) {
    auto resp = out_queue.pop();
    if (resp->context()->id() == "#schedule") {
      ASSERT_FALSE(resp->succeeded());
    }
    else if (resp->context()->id() == "#metrics" ||
             resp->context()->id() == "#interrupt") {
      ASSERT_TRUE(resp->succeeded());
    }
    else {
      ASSERT_TRUE(false) << "Wrong SwmUID";
    }
  }
}

TEST_F(ctrl, processor_exchange) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > in_queue(1);
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > out_queue(5);
  {
    swm::util::Processor processor;
    ASSERT_NO_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
    in_queue.push(create_schedule_request("#schedule1", { "swm-fcfs", "swm-dummy" },
                                          SchedulingInfoPresets::one_node_one_job("hold_on")));
    in_queue.push(create_schedule_request("#schedule2", { "swm-fcfs", "swm-dummy" },
                                          SchedulingInfoPresets::one_node_one_job("hold_on")));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    in_queue.push(create_exchange_request("#exchange", "#schedule1", "#schedule2"));
    in_queue.push(create_interrupt_request("#interrupt1", "#schedule1"));
    in_queue.push(create_interrupt_request("#interrupt2", "#schedule2"));
  }
  ASSERT_EQ(out_queue.element_count(), 5);
  while (out_queue.element_count() > 0) {
    auto resp = out_queue.pop();
    if (resp->context()->id() == "#schedule1" || resp->context()->id() == "#schedule2") {
      ASSERT_FALSE(resp->succeeded());
    }
    else if (resp->context()->id() == "#exchange" ||
             resp->context()->id() == "#interrupt1" ||
             resp->context()->id() == "#interrupt2") {
      ASSERT_TRUE(resp->succeeded());
    }
  }
}

TEST_F(ctrl, processor_uid_conflict) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > in_queue(1);
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > out_queue(3);
  {
    swm::util::Processor processor;
    ASSERT_NO_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
    in_queue.push(create_schedule_request("#schedule", { "swm-dummy" },
                                          SchedulingInfoPresets::one_node_one_job("hold_on")));
    in_queue.push(create_schedule_request("#schedule", { "swm-dummy" },
                                          SchedulingInfoPresets::one_node_one_job("hold_on")));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    in_queue.push(create_interrupt_request("#interrupt", "#schedule"));
  }
  ASSERT_EQ(out_queue.element_count(), 3);
  while (out_queue.element_count() > 0) {
    auto resp = out_queue.pop();
    if (resp->context()->id() == "#schedule") {
      ASSERT_FALSE(resp->succeeded());
    }
    else if (resp->context()->id() == "#interrupt") {
      ASSERT_TRUE(resp->succeeded());
    }
    else {
      ASSERT_TRUE(false) << "Wrong SwmUID";
    }
  }
}

TEST_F(ctrl, processor_no_such_uid) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > in_queue(1);
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > out_queue(3);
  {
    swm::util::Processor processor;
    ASSERT_NO_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
    in_queue.push(create_interrupt_request("#interrupt", "#schedule"));
    in_queue.push(create_metrics_request("#metrics", "#schedule"));
    in_queue.push(create_exchange_request("#exchange", "#schedule1", "#schedule2"));
  }
  ASSERT_EQ(out_queue.element_count(), 3);
  while (out_queue.element_count() > 0) {
    auto resp = out_queue.pop();
    auto name = resp->context()->id();
    ASSERT_TRUE(name == "#interrupt" || name == "#metrics" || name == "#exchange");
    ASSERT_FALSE(resp->succeeded());
  }
}

TEST_F(ctrl, processor_time_counting) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > in_queue(1);
  swm::util::MyQueue<std::shared_ptr<swm::util::ResponseInterface> > out_queue(2);
  {
    swm::util::Processor processor;
    ASSERT_NO_THROW(processor.init(factory(), scanner(), &in_queue, &out_queue, 10.0));
    in_queue.push(create_schedule_request("#schedule", { "swm-dummy" },
                                          SchedulingInfoPresets::one_node_one_job("hold_on")));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    in_queue.push(create_interrupt_request("#interrupt", "#schedule"));
  }
  ASSERT_EQ(out_queue.element_count(), 2);
  while (out_queue.element_count() > 0) {
    auto resp = out_queue.pop();
    double astro = 0.0, idling = 0.0, working = 0.0;
    resp->context()->timer()->get_times(&astro, &idling, &working);
    ASSERT_GE(astro, 0.0);
    ASSERT_GE(working, 0.0);

    if (resp->context()->id() == "#schedule") {
      ASSERT_FALSE(resp->succeeded());
      ASSERT_GE(astro, 0.025); ASSERT_LE(astro, 0.075);
      ASSERT_GE(working, 0.025); ASSERT_LE(working, 0.075);
      ASSERT_LE(idling, 0.010);
    }
    else if (resp->context()->id() == "#interrupt") {
      ASSERT_TRUE(resp->succeeded());
      ASSERT_LE(astro, 0.010);
      ASSERT_LE(idling, 0.010);
      ASSERT_LE(working, 0.010);
    }
    else {
      ASSERT_TRUE(false) << "Wrong SwmUID";
    }
  }
}

