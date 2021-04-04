#pragma once

#include <gtest/gtest.h>
#include <algorithm>

#include "test_defs.h"
#include "plg.h"
#include "scheduling_info_configurator.h"


TEST_F(plg, fcfs_default_case) {
  for (auto ignore_priorities : { false, true }) {
    SchedulingInfoConfigurator config;
    auto cluster = config.create_cluster("1", "up");
    auto part = cluster->create_partition("1", "up");
    part->create_node("1", "up", "idle");
    part->create_node("2", "up", "idle");
    part->create_node("3", "up", "idle");
    auto job1 = config.create_job("1", "1", 1); job1->create_request("node", 2);
    auto job2 = config.create_job("2", "1", 1); job2->create_request("node", 1);
    auto job3 = config.create_job("3", "1", 1); job3->create_request("node", 1);
    auto job4 = config.create_job("4", "1", 1); job4->create_request("node", 3);
    auto job5 = config.create_job("5", "1", 1); job5->create_request("node", 1);
    std::shared_ptr<swm::SchedulingInfoInterface> info;
    config.construct(&info);

    swm::FcfsImplementation fcfs;
    std::vector<swm::SwmTimetable> tts;
    ASSERT_TRUE(fcfs.init(info.get()));
    ASSERT_TRUE(fcfs.schedule(info->jobs(), events(), &tts, ignore_priorities));

    std::sort(tts.begin(), tts.end(), [](const swm::SwmTimetable &tt1,
                                         const swm::SwmTimetable &tt2) -> bool {
      return tt1.get_job_id() < tt2.get_job_id();
    });
    ASSERT_EQ(tts[0].get_start_time(), 0); ASSERT_EQ(tts[0].get_job_nodes().size(), 2);
    ASSERT_EQ(tts[1].get_start_time(), 0); ASSERT_EQ(tts[1].get_job_nodes().size(), 1);
    ASSERT_EQ(tts[2].get_start_time(), 1); ASSERT_EQ(tts[2].get_job_nodes().size(), 1);
    ASSERT_EQ(tts[3].get_start_time(), 2); ASSERT_EQ(tts[3].get_job_nodes().size(), 3);
    ASSERT_EQ(tts[4].get_start_time(), 3); ASSERT_EQ(tts[4].get_job_nodes().size(), 1);
  }
}

TEST_F(plg, fcfs_wrong_dependencies) {
  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  part->create_node("1", "up", "idle");
  auto job1 = config.create_job("1", "1", 1); job1->create_request("node", 1);
  auto job2 = config.create_job("2", "1", 1);
  job2->create_request("node", 1);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  swm::FcfsImplementation fcfs;
  std::vector<swm::SwmTimetable> tts;
  ASSERT_TRUE(fcfs.init(info.get()));
  ASSERT_TRUE(fcfs.schedule(info->jobs(), events(), &tts, true));
  ASSERT_EQ(tts.size(), 2);
  ASSERT_NE(tts[0].get_job_nodes().size(), 0);
  ASSERT_NE(tts[1].get_job_nodes().size(), 0);  
  
  tts.clear();
  job1->set_dependency("2");
  config.construct(&info);
  ASSERT_TRUE(fcfs.schedule(info->jobs(), events(), &tts, true));
  ASSERT_EQ(tts.size(), 1);
  ASSERT_EQ(tts[0].get_job_id(), "2");
  ASSERT_NE(tts[0].get_job_nodes().size(), 0);
}

TEST_F(plg, fcfs_priorities) {
  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  part->create_node("1", "up", "idle");
  
  auto job1 = config.create_job("1", "1", 1);
  job1->create_request("node", 1); job1->set_priority(10);
  
  auto job2 = config.create_job("2", "1", 2);
  job2->create_request("node", 1); job2->set_priority(20);
  
  auto job3 = config.create_job("3", "1", 3);
  job3->create_request("node", 1); job3->set_priority(0);

  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  swm::FcfsImplementation fcfs;
  std::vector<swm::SwmTimetable> tts;
  ASSERT_TRUE(fcfs.init(info.get()));
  ASSERT_TRUE(fcfs.schedule(info->jobs(), events(), &tts, false));
  ASSERT_EQ(tts.size(), 3);
  std::sort(tts.begin(), tts.end(), [](const swm::SwmTimetable &tt1,
                                       const swm::SwmTimetable &tt2) -> bool {
    return tt1.get_job_id() < tt2.get_job_id();
  });
  ASSERT_EQ(tts[0].get_start_time(), 2); ASSERT_EQ(tts[0].get_job_nodes().size(), 1);
  ASSERT_EQ(tts[1].get_start_time(), 0); ASSERT_EQ(tts[1].get_job_nodes().size(), 1);
  ASSERT_EQ(tts[2].get_start_time(), 3); ASSERT_EQ(tts[2].get_job_nodes().size(), 1);
}
