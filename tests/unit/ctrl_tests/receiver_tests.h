
#pragma once

#include <gtest/gtest.h>
#include <fstream>

#include "test_defs.h"
#include "ctrl.h"
#include "ctrl/receiver.h"


TEST_F(ctrl, receiver_wrong_init) {
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > queue(1);
  std::unique_ptr<swm::util::Receiver> receiver;
  receiver.reset(new swm::util::Receiver());
  ASSERT_ANY_THROW(receiver->init(&queue, nullptr));
  receiver.reset(new swm::util::Receiver());
  ASSERT_ANY_THROW(receiver->init(nullptr, &std::cin));
}

TEST_F(ctrl, receiver_second_init) {
  std::istringstream istr1, istr2;
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > queue(3);
  swm::util::Receiver receiver;
  ASSERT_NO_THROW(receiver.init(&queue, &istr1));
  while (!receiver.finished() && queue.element_count() < queue.size()) {
    std::this_thread::yield();
  }
  ASSERT_ANY_THROW(receiver.init(&queue, &istr2));
}

TEST_F(ctrl, receiver_no_init) {
  swm::util::Receiver receiver;
  ASSERT_ANY_THROW(receiver.finished());
}

TEST_F(ctrl, receiver_wrong_config) {
  std::istringstream istr;
  istr.str("No one expects the Spanish inquisition!");
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > queue(2);
  swm::util::Receiver receiver;
  ASSERT_NO_THROW(receiver.init(&queue, &istr));

  while (!receiver.finished() && queue.element_count() < queue.size()) {
    std::this_thread::yield();
  }
  ASSERT_EQ(queue.element_count(), 0);
}

TEST_F(ctrl, receiver_parse_jobs) {
  std::string json;
  construct_sched_command(&json);
  std::shared_ptr<swm::util::CommandInterface> cmd;
  receive_single_sched_command(json, &cmd);
  ASSERT_NE(cmd.get(), nullptr);
  auto scmd = static_cast<swm::util::ScheduleCommand *>(cmd.get());

  ASSERT_EQ(scmd->scheduling_info()->jobs().size(), 1);
  auto job = scmd->scheduling_info()->jobs()[0];
  ASSERT_EQ(job->get_id(), "10000000-0000-0000-0000-000000000000");
  ASSERT_EQ(job->get_cluster_id(), "1");
  ASSERT_EQ(job->get_state(), "Q");

  auto resources = job->get_request();
  ASSERT_EQ(resources.size(), 2);
  std::sort(resources.begin(), resources.end(),
            name_comparator<const swm::SwmResource>());
  auto res1 = resources[0];
  ASSERT_EQ(res1.get_name(), "node");
  ASSERT_EQ(res1.get_count(), 3);
  auto res2 = resources[1];
  ASSERT_EQ(res2.get_name(), "mem");
  ASSERT_EQ(res2.get_count(), 1073741824);
}

TEST_F(ctrl, receiver_parse_cluster) {
  std::string json;
  construct_sched_command(&json);
  std::shared_ptr<swm::util::CommandInterface> cmd;
  receive_single_sched_command(json, &cmd);
  ASSERT_NE(cmd.get(), nullptr);
  auto scmd = static_cast<swm::util::ScheduleCommand *>(cmd.get());

  auto sched = scmd->scheduling_info();
  ASSERT_EQ(sched->clusters().size(), 1);
  auto cluster = sched->clusters()[0];
  ASSERT_EQ(cluster->get_id(), "1");
  ASSERT_EQ(cluster->get_state(), "up");
  ASSERT_EQ(cluster->get_scheduler(), 1);
}

TEST_F(ctrl, receiver_parse_partitions) {
  std::string json;
  construct_sched_command(&json);
  std::shared_ptr<swm::util::CommandInterface> cmd;
  receive_single_sched_command(json, &cmd);
  ASSERT_NE(cmd.get(), nullptr);
  auto scmd = static_cast<swm::util::ScheduleCommand *>(cmd.get());

  auto sched = scmd->scheduling_info();
  ASSERT_EQ(sched->parts().size(), 2);
  auto parts = sched->parts();
  std::sort(parts.begin(), parts.end(),
            id_comparator<const swm::SwmPartition *>());
  auto part1 = parts[0];
  ASSERT_EQ(part1->get_id(), "1");
  ASSERT_EQ(part1->get_state(), "down");
  ASSERT_EQ(part1->get_jobs_per_node(), 1);

  auto part2 = parts[1];
  ASSERT_EQ(part2->get_id(), "2");
  ASSERT_EQ(part2->get_state(), "up");
  ASSERT_EQ(part2->get_jobs_per_node(), 2);
}

TEST_F(ctrl, receiver_parse_nodes) {
  std::string json;
  construct_sched_command(&json);
  std::shared_ptr<swm::util::CommandInterface> cmd;
  receive_single_sched_command(json, &cmd);
  ASSERT_NE(cmd.get(), nullptr);
  auto scmd = static_cast<swm::util::ScheduleCommand *>(cmd.get());

  auto sched = scmd->scheduling_info();
  ASSERT_EQ(sched->nodes().size(), 2);
  auto nodes = sched->nodes();
  std::sort(nodes.begin(), nodes.end(),
            id_comparator<const swm::SwmNode *>());

  auto node1 = nodes[0];
  ASSERT_EQ(node1->get_id(), "1");
  ASSERT_EQ(node1->get_state_power(), "up");
  ASSERT_EQ(node1->get_state_alloc(), "free");
  ASSERT_EQ(node1->get_resources().size(), 2);
  auto resources = node1->get_resources();
  std::sort(resources.begin(), resources.end(),
            name_comparator<const swm::SwmResource>());
  auto node1_res1 = resources[0];
  ASSERT_EQ(node1_res1.get_name(), "cpu");
  ASSERT_EQ(node1_res1.get_count(), 24);
  auto node1_res2 = resources[1];
  ASSERT_EQ(node1_res2.get_name(), "mem");
  ASSERT_EQ(node1_res2.get_count(), 68719476736);
  std::sort(nodes.begin(), nodes.end(),
            id_comparator<const swm::SwmNode *>());
  auto node2 = nodes[1];
  ASSERT_EQ(node2->get_id(), "3");
  ASSERT_EQ(node2->get_state_power(), "down");
}

TEST_F(ctrl, receiver_parse_rh) {
  std::string json;
  construct_sched_command(&json);
  std::shared_ptr<swm::util::CommandInterface> cmd;
  receive_single_sched_command(json, &cmd);
  ASSERT_NE(cmd.get(), nullptr);
  auto scmd = static_cast<swm::util::ScheduleCommand *>(cmd.get());

  auto sched = scmd->scheduling_info();
  ASSERT_EQ(sched->resource_hierarchy().size(), 1);
  auto rh1 = sched->resource_hierarchy()[0];
  ASSERT_EQ(rh1.name(), "cluster");
  ASSERT_EQ(rh1.id(), "1");
  ASSERT_EQ(rh1.children().size(), 2);

  auto rh2 = rh1.children()[0];
  ASSERT_EQ(rh2.name(), "partition");
  ASSERT_EQ(rh2.id(), "1");
  ASSERT_EQ(rh2.children().size(), 0);

  auto rh3 = rh1.children()[1];
  ASSERT_EQ(rh3.name(), "partition");
  ASSERT_EQ(rh3.id(), "2");
  ASSERT_EQ(rh3.children().size(), 2);

  auto rh4 = rh3.children()[0];
  ASSERT_EQ(rh4.name(), "node");
  ASSERT_EQ(rh4.id(), "1");
  ASSERT_EQ(rh4.children().size(), 0);

  auto rh5 = rh3.children()[1];
  ASSERT_EQ(rh5.name(), "node");
  ASSERT_EQ(rh5.id(), "3");
  ASSERT_EQ(rh5.children().size(), 0);
}
