#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "alg/algorithm_factory.h"

TEST(alg, algorithm_factory_load_plugins_default) {
  swm::AlgorithmFactory factory;
  std::stringstream error;
  ASSERT_TRUE(factory.load_plugins(find_plugin_dir(), &error)) << error.str();
}

TEST(alg, algorithm_factory_reload_plugins) {
  swm::AlgorithmFactory factory;
  std::string plugin_dir = find_plugin_dir();
  factory.load_plugins(plugin_dir);
  ASSERT_FALSE(factory.load_plugins(plugin_dir));
}

TEST(alg, algorithm_factory_load_plugins_wrong_path) {
  swm::AlgorithmFactory factory;
  ASSERT_FALSE(factory.load_plugins("wrong_path"));
}

TEST(alg, algorithm_factory_load_plugins_wrong_plugins) {
  swm::AlgorithmFactory factory;
#if defined(WIN32)
  std::string plugin_dir = find_win_dir() + "..\\..\\Program Files\\Internet explorer";
#else
  std::string plugin_dir = "/usr/lib/sudo";
#endif
  ASSERT_FALSE(factory.load_plugins(plugin_dir));
}

TEST(alg, algorithm_factory_known_algorithms_default) {
  swm::AlgorithmFactory factory;
  ASSERT_TRUE(factory.load_plugins(find_plugin_dir()));
  ASSERT_FALSE(factory.known_algorithms().empty());
}

TEST(alg, algorithm_factory_default_algorithms_found) {
  swm::AlgorithmFactory factory;
  ASSERT_TRUE(factory.load_plugins(find_plugin_dir()));
  const auto &algs = factory.known_algorithms();
  std::vector<std::string> ids = { "swm-fcfs" };
  auto pred = [&algs](std::string &id) {
    for (const auto &alg: algs)
      if (id == alg->family_id()) {
        return true;
      }
    return false;
  };
  ASSERT_TRUE(std::all_of(ids.begin(), ids.end(), pred));
}

TEST(alg, algorithm_factory_create_fcfs) {
  swm::AlgorithmFactory factory;
  ASSERT_TRUE(factory.load_plugins(find_plugin_dir()));
  const auto &algs = factory.known_algorithms();
  auto pred = [](const swm::AlgorithmDescInterface* alg) { 
    return alg->family_id() == "swm-fcfs"; 
  };
  auto fcfs_lib = std::find_if(algs.begin(), algs.end(), pred);
  ASSERT_TRUE(fcfs_lib != algs.end());
  std::stringstream error;
  std::shared_ptr<swm::Algorithm> res;
  ASSERT_TRUE(factory.create(*fcfs_lib, &res, &error)) << error.str();
}

TEST(alg, algorithm_factory_create_desc_nullptr) {
  swm::AlgorithmFactory factory;
  ASSERT_TRUE(factory.load_plugins(find_plugin_dir()));
  std::shared_ptr<swm::Algorithm> res;
  ASSERT_ANY_THROW(factory.create(nullptr, &res));
}
