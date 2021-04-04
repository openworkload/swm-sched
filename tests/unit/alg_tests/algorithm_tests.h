#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "alg/algorithm.h"
#include "alg/algorithm_factory.h"
#include "ctrl/scheduling_info.h"
#include "scheduling_info_configurator.h"

class alg_sched : public ::testing::Test {

 protected:

  void SetUp() {
    ASSERT_TRUE(factory_.load_plugins(find_plugin_dir()));
    const auto &plugins = factory_.known_algorithms();
    ASSERT_FALSE(plugins.empty());

    for (const auto &plugin : plugins) {
      if (plugin->family_id() != "swm-dummy") {
        algorithms_.push_back(plugin);
      }
    }
  }
  void TearDown() { }

  swm::PluginEventsInterface *events() { return &events_; }
  bool create_algorithms(std::vector<std::shared_ptr<swm::Algorithm> > *algs) {
    algs->clear();
    algs->reserve(algorithms_.size());
    for (auto &desc : algorithms_) {
      std::shared_ptr<swm::Algorithm> algorithm;
      if (!factory_.create(desc, &algorithm)) {
        return false;
      }
      algs->emplace_back(algorithm);
    }
    return true;
  }

  bool all_jobs_scheduled(std::vector<const swm::SwmTimetable *> *tts, size_t expected_count) {
    if (tts->size() != expected_count) {
      return false;
    }

    std::sort(tts->begin(), tts->end(), [](const swm::SwmTimetable *t1,
                                           const swm::SwmTimetable *t2) -> bool {
      return t1->get_job_id() < t2->get_job_id();
    });
    for (size_t i = 0; i < tts->size(); i++) {
      auto tt = (*tts)[i];
      if (tt->get_job_id().empty() || tt->get_job_nodes().empty()) {
        return false;
      }
    }

    return true;
  }

 private:
  EmptyPluginEvents events_;
  std::vector<const swm::AlgorithmDescInterface *> algorithms_;
  swm::AlgorithmFactory factory_;
};

TEST_F(alg_sched, algorithm_empty_sched) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));
  
  SchedulingInfoConfigurator config;
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    ASSERT_TRUE(tt->tables().empty());
  }
}

TEST_F(alg_sched, algorithm_no_job) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));
  
  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  auto node = part->create_node("1", "up", "idle");
  node->create_resources({ { "cpu", 32 },{ "mem", 68719476736 } });
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    ASSERT_TRUE(tt->tables().empty());
  }
}

TEST_F(alg_sched, algorithm_no_node) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  cluster->create_partition("1", "up");
  auto job = config.create_job("1", "1", 0);
  job->create_request("node", 3);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_FALSE(all_jobs_scheduled(&tts, 1));
  }
}

TEST_F(alg_sched, algorithm_default) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  auto node = part->create_node("1", "up", "idle");
  node->create_resources({ { "cpu", 32 },{ "mem", 68719476736 } });
  auto job = config.create_job("1", "1", 0);
  job->create_request("node", 1);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 1));
    ASSERT_EQ(tts[0]->get_job_id(), "1");
    ASSERT_EQ(tts[0]->get_start_time(), 0);
    ASSERT_EQ(tts[0]->get_job_nodes().size(), 1);
    ASSERT_EQ(tts[0]->get_job_nodes()[0], "1");
  }
}

TEST_F(alg_sched, algorithm_no_resource) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  auto node = part->create_node("1", "up", "idle");
  node->create_resource("cpu", 32);
  auto job = config.create_job("1", "1", 0);
  job->create_requests( { { "node", 1 }, { "mem", 68719476736 } } );
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_FALSE(all_jobs_scheduled(&tts, 1));
  }
}

TEST_F(alg_sched, algorithm_not_enough_memory) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  auto node = part->create_node("1", "up", "idle");
  node->create_resources( { { "cpu", 32 },{ "mem", 34359738368 } } );
  auto job = config.create_job("1", "1", 0);
  job->create_requests( { { "node", 1 }, { "mem", 68719476736 } } );
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_FALSE(all_jobs_scheduled(&tts, 1));
  }
}

TEST_F(alg_sched, algorithm_not_enough_nodes) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  auto node = part->create_node("1", "up", "idle");
  node->create_resources( { { "cpu", 32 }, { "mem", 68719476736 } } );
  auto job = config.create_job("1", "1", 0);
  job->create_request("node", 2);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_FALSE(all_jobs_scheduled(&tts, 1));
  }
}

TEST_F(alg_sched, algorithm_empty_job_request) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  auto node = part->create_node("1", "up", "idle");
  node->create_resources( { { "cpu", 32 }, { "mem", 34359738368 } } );
  config.create_job("1", "1", 0);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    ASSERT_EQ(tt->tables().size(), 0);
  }
}

TEST_F(alg_sched, algorithm_not_queued_jobs) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  part->create_node("1", "up", "idle");
  auto job1 = config.create_job("1", "1", 0); job1->create_request("node", 1); job1->set_state("S");
  auto job2 = config.create_job("2", "1", 0); job2->create_request("node", 1);
  auto job3 = config.create_job("3", "1", 0); job3->create_request("node", 1); job3->set_state("C");
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 1));
    ASSERT_EQ(tts[0]->get_job_id(), "2"); ASSERT_EQ(tts[0]->get_job_nodes().size(), 1);
  }
}

TEST_F(alg_sched, algorithm_failed_and_succeeded_jobs) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  auto node = part->create_node("1", "up", "idle");
  node->create_resource("mem", 1);
  auto job1 = config.create_job("1", "1", 1);
  job1->create_request("node", 1); job1->create_request("mem", 1);
  auto job2 = config.create_job("2", "1", 1);
  job2->create_request("node", 1); job2->create_request("mem", 2);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 1));
    ASSERT_EQ(tts[0]->get_job_id(), "1");
  }
}

TEST_F(alg_sched, algorithm_2_jobs_1_node) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  part->create_node("1", "up", "idle");
  auto job1 = config.create_job("1", "1", 10);
  job1->create_request("node", 1);
  auto job2 = config.create_job("2", "1", 10);
  job2->create_request("node", 1);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 2));
    ASSERT_EQ(tts[0]->get_job_id(), "1");
    ASSERT_EQ(tts[0]->get_start_time(), 0);
    ASSERT_EQ(tts[1]->get_job_id(), "2");
    ASSERT_EQ(tts[1]->get_start_time(), 10);
  }
}

TEST_F(alg_sched, algorithm_5_jobs_2_nodes) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  part->create_node("1", "up", "idle"); part->create_node("2", "up", "idle");
  auto job1 = config.create_job("1", "1", 1); job1->create_request("node", 1);
  auto job2 = config.create_job("2", "1", 1); job2->create_request("node", 1);
  auto job3 = config.create_job("3", "1", 1); job3->create_request("node", 2);
  auto job4 = config.create_job("4", "1", 1); job4->create_request("node", 1);
  auto job5 = config.create_job("5", "1", 1); job5->create_request("node", 2);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 5));
    std::map<uint64_t, size_t> nodes_per_tick;
    for (const auto job : tts) {
      nodes_per_tick[job->get_start_time()] += job->get_job_nodes().size();
    }
    for (const auto rec : nodes_per_tick) {
      ASSERT_GT(rec.second, 0); ASSERT_LE(rec.second, 2);
    }
  }
}

TEST_F(alg_sched, algorithm_same_partition) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part1 = cluster->create_partition("1", "up");
  part1->create_node("1", "up", "idle");
  auto part2 = cluster->create_partition("2", "up");
  part2->create_node("2", "up", "idle");
  part2->create_node("3", "up", "idle");
  part2->create_node("4", "up", "idle");
  auto part3 = cluster->create_partition("3", "up");
  part3->create_node("5", "up", "idle");
  auto job = config.create_job("1", "1", 0);
  job->create_request("node", 3);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 1));
    ASSERT_EQ(tts[0]->get_job_id(), "1");
    auto nodes = tts[0]->get_job_nodes();
    ASSERT_EQ(nodes.size(), 3);
    std::sort(nodes.begin(), nodes.end());
    ASSERT_EQ(nodes[0], "2");
    ASSERT_EQ(nodes[1], "3");
    ASSERT_EQ(nodes[2], "4");
  }
}

TEST_F(alg_sched, algorithm_someone_down) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part1 = cluster->create_partition("1", "up");
  part1->create_node("1", "down", "");
  part1->create_node("2", "up", "idle");
  auto part2 = cluster->create_partition("2", "down");
  part2->create_node("3", "up", "idle");
  part2->create_node("4", "up", "idle");
  auto part3 = cluster->create_partition("3", "up");
  part3->create_node("5", "up", "idle");
  part3->create_node("6", "down", "");
  auto job = config.create_job("1", "1", 0);
  job->create_request("node", 2);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 1));
    auto nodes = tts[0]->get_job_nodes();
    ASSERT_EQ(nodes.size(), 2);
    std::sort(nodes.begin(), nodes.end());
    ASSERT_EQ(nodes[0], "2");
    ASSERT_EQ(nodes[1], "5");
  }
}

TEST_F(alg_sched, algorithm_different_clusters) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster1 = config.create_cluster("1", "up");
  auto part1 = cluster1->create_partition("1", "up");
  part1->create_node("1", "up", "idle");
  auto cluster2 = config.create_cluster("2", "up");
  auto part2 = cluster2->create_partition("2", "up");
  part2->create_node("2", "up", "idle");
  auto job1 = config.create_job("1", "1", 1); job1->create_request("node", 1);
  auto job2 = config.create_job("2", "2", 2); job2->create_request("node", 1);
  auto job3 = config.create_job("3", "2", 1); job3->create_request("node", 1);
  auto job4 = config.create_job("4", "1", 1); job4->create_request("node", 1);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 4));
    for (const auto table : tts) { ASSERT_EQ(table->get_job_nodes().size(), 1); }
    ASSERT_EQ(tts[0]->get_job_nodes()[0], "1"); ASSERT_EQ(tts[0]->get_start_time(), 0);
    ASSERT_EQ(tts[1]->get_job_nodes()[0], "2"); ASSERT_EQ(tts[1]->get_start_time(), 0);
    ASSERT_EQ(tts[2]->get_job_nodes()[0], "2"); ASSERT_EQ(tts[2]->get_start_time(), 2);
    ASSERT_EQ(tts[3]->get_job_nodes()[0], "1"); ASSERT_EQ(tts[3]->get_start_time(), 1);
  }
}

TEST_F(alg_sched, algorithm_job_dependencies) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster1 = config.create_cluster("1", "up");
  auto part1 = cluster1->create_partition("1", "up");
  part1->create_node("1", "up", "idle");
  part1->create_node("2", "up", "idle");
  auto job1 = config.create_job("1", "1", 2); job1->create_request("node", 1);
  auto job2 = config.create_job("2", "1", 2); job2->create_request("node", 1);
  job2->set_dependency("1");
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 2));
    ASSERT_EQ(tts[0]->get_start_time(), 0);
    ASSERT_EQ(tts[1]->get_start_time(), 2);
  }
}

TEST_F(alg_sched, algorithm_gang_id) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  part->create_node("1", "up", "idle");
  part->create_node("2", "up", "idle");
  auto job1 = config.create_job("1", "1", 1);job1->create_request("node", 1);
  auto job2 = config.create_job("2", "1", 2);job2->create_request("node", 1);job2->set_gang_id("1");
  auto job3 = config.create_job("3", "1", 3);job3->create_request("node", 1);job3->set_gang_id("1");
  auto job4 = config.create_job("4", "1", 1);job4->create_request("node", 1);job4->set_gang_id("2");
  auto job5 = config.create_job("5", "1", 1);job5->create_request("node", 1);job5->set_gang_id("2");

  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 5));

    ASSERT_EQ(tts[0]->get_start_time(), 0);
    ASSERT_EQ(tts[1]->get_start_time(), 1);
    ASSERT_EQ(tts[2]->get_start_time(), 1);
    ASSERT_EQ(tts[3]->get_start_time(), 4);
    ASSERT_EQ(tts[4]->get_start_time(), 4);
  }
}

TEST_F(alg_sched, algorithm_metrics) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  auto node = part->create_node("1", "up", "idle");
  node->create_resources( { { "cpu", 32 }, { "mem", 68719476736 } } );
  auto job = config.create_job("1", "1", 0);
  job->create_request("node", 1);
  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_EQ(alg->algorithm_metrics().scheduled_jobs(), 0);
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    ASSERT_EQ(alg->algorithm_metrics().scheduled_jobs(), 1);
  }
}

TEST_F(alg_sched, preset_job_nodes_available) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");

  auto node1 = part->create_node("1", "up", "idle");
  node1->create_resources({ { "cpu", 32 },{ "mem", 68719476736 } });
  auto node2 = part->create_node("2", "up", "idle");
  node2->create_resources({ { "cpu", 8 },{ "mem", 68719476736 } });
  auto node3 = part->create_node("3", "up", "busy");
  node3->create_resources({ { "cpu", 8 },{ "mem", 68719476736 } });
  auto node4 = part->create_node("4", "up", "idle");
  node4->create_resources({ { "cpu", 8 },{ "mem", 68719476736 } });

  auto job = config.create_job("1", "1", 0);
  job->create_request("node", 2);
  job->set_node_ids({"2", "4"});

  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 1));
    ASSERT_EQ(tts[0]->get_job_id(), "1");
    ASSERT_EQ(tts[0]->get_start_time(), 0);
    ASSERT_EQ(tts[0]->get_job_nodes().size(), 2);
    ASSERT_EQ(tts[0]->get_job_nodes()[0], "2");
    ASSERT_EQ(tts[0]->get_job_nodes()[1], "4");
  }
}

TEST_F(alg_sched, preset_job_nodes_not_available) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");

  auto node1 = part->create_node("1", "up", "idle");
  node1->create_resources({ { "cpu", 32 },{ "mem", 68719476736 } });
  auto node2 = part->create_node("2", "up", "idle");
  node2->create_resources({ { "cpu", 8 },{ "mem", 68719476736 } });
  auto node3 = part->create_node("3", "up", "busy");
  node3->create_resources({ { "cpu", 8 },{ "mem", 68719476736 } });
  auto node4 = part->create_node("4", "up", "idle");
  node4->create_resources({ { "cpu", 8 },{ "mem", 68719476736 } });

  auto job = config.create_job("1", "1", 0);
  job->create_request("node", 2);
  job->set_node_ids({"1", "3"});

  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_FALSE(all_jobs_scheduled(&tts, 1));
    ASSERT_EQ(tt->tables().size(), 0);
  }
}

TEST_F(alg_sched, jobs_with_same_preset_nodes) {
  std::vector<std::shared_ptr<swm::Algorithm> > algs;
  ASSERT_TRUE(create_algorithms(&algs));

  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");

  auto node1 = part->create_node("1", "up", "idle");
  node1->create_resources({ { "cpu", 32 },{ "mem", 68719476736 } });
  auto node2 = part->create_node("2", "up", "idle");
  node2->create_resources({ { "cpu", 8 },{ "mem", 68719476736 } });
  auto node3 = part->create_node("3", "up", "idle");
  node3->create_resources({ { "cpu", 8 },{ "mem", 68719476736 } });

  auto job1 = config.create_job("1", "1", 1000);
  job1->create_request("node", 1);
  job1->set_node_ids({"1"});

  auto job2 = config.create_job("2", "1", 1000);
  job2->create_request("node", 2);
  job2->set_node_ids({"1", "2"});

  auto job3 = config.create_job("3", "1", 1000);
  job3->create_request("node", 2);
  job3->set_node_ids({"2", "3"});

  std::shared_ptr<swm::SchedulingInfoInterface> info;
  config.construct(&info);

  for (auto &alg : algs) {
    std::shared_ptr<swm::TimetableInfoInterface> tt;
    ASSERT_TRUE(alg->create_timetable(info.get(), events(), &tt));
    auto tts = tt->tables();
    ASSERT_TRUE(all_jobs_scheduled(&tts, 3));

    ASSERT_EQ(tts[0]->get_job_id(), "1");
    ASSERT_EQ(tts[0]->get_start_time(), 0);
    ASSERT_EQ(tts[0]->get_job_nodes().size(), 1);
    ASSERT_EQ(tts[0]->get_job_nodes()[0], "1");

    ASSERT_EQ(tts[1]->get_job_id(), "2");
    ASSERT_EQ(tts[1]->get_start_time(), 1000);
    ASSERT_EQ(tts[1]->get_job_nodes().size(), 2);
    ASSERT_EQ(tts[1]->get_job_nodes()[0], "2");
    ASSERT_EQ(tts[1]->get_job_nodes()[1], "1");

    ASSERT_EQ(tts[2]->get_job_id(), "3");
    ASSERT_EQ(tts[2]->get_start_time(), 2000);
    ASSERT_EQ(tts[2]->get_job_nodes().size(), 2);
    ASSERT_EQ(tts[2]->get_job_nodes()[0], "3");
    ASSERT_EQ(tts[2]->get_job_nodes()[1], "2");
  }
}
