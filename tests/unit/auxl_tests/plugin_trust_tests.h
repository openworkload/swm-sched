#pragma once

#include "alg/algorithm_factory.h"
#include "auxl/plugin_trust.h"
#include "helpers.h"
#include "test_defs.h"

#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <string>

namespace {

std::string make_temp_subdir(const std::string &suffix) {
  std::string path = find_temp_dir() + "/swm-sched-plugin-trust-" + suffix + "-" + std::to_string(getpid());
  if (mkdir(path.c_str(), 0755) != 0) {
    // Retry once if left over from a previous crashed run.
    rmdir(path.c_str());
    EXPECT_EQ(mkdir(path.c_str(), 0755), 0);
  }
  return path;
}

void write_empty_file(const std::string &path, mode_t mode) {
  std::ofstream out(path, std::ios::binary);
  out << "x";
  out.close();
  ASSERT_EQ(chmod(path.c_str(), mode), 0);
}

}  // namespace

TEST(plugin_trust, basename_allowed) {
  EXPECT_TRUE(swm::util::plugin_basename_allowed("swm-sched-fcfs.so"));
  EXPECT_TRUE(swm::util::plugin_basename_allowed("swm-sched-dummy.so"));
  EXPECT_TRUE(swm::util::plugin_basename_allowed("swm-sched-fair-sharing.so"));
  EXPECT_FALSE(swm::util::plugin_basename_allowed("evil.so"));
  EXPECT_FALSE(swm::util::plugin_basename_allowed("swm-sched-fcfs.so.bak"));
  EXPECT_FALSE(swm::util::plugin_basename_allowed(""));
}

TEST(plugin_trust, basename_from_path) {
  EXPECT_EQ(swm::util::plugin_basename("/opt/lib64/swm-sched-fcfs.so"), "swm-sched-fcfs.so");
  EXPECT_EQ(swm::util::plugin_basename("swm-sched-fcfs.so"), "swm-sched-fcfs.so");
}

TEST(plugin_trust, directory_trusted_modes) {
  const std::string dir = make_temp_subdir("dir");
  std::stringstream errors;
  EXPECT_TRUE(swm::util::plugin_directory_trusted(dir, &errors)) << errors.str();

  ASSERT_EQ(chmod(dir.c_str(), 0777), 0);
  errors.str("");
  EXPECT_FALSE(swm::util::plugin_directory_trusted(dir, &errors));
  EXPECT_FALSE(errors.str().empty());

  ASSERT_EQ(chmod(dir.c_str(), 0755), 0);
  rmdir(dir.c_str());
}

TEST(plugin_trust, file_path_trusted_modes) {
  const std::string dir = make_temp_subdir("file");
  const std::string good = dir + "/swm-sched-fcfs.so";
  write_empty_file(good, 0644);

  std::stringstream errors;
  EXPECT_TRUE(swm::util::plugin_file_path_trusted(good, &errors)) << errors.str();

  ASSERT_EQ(chmod(good.c_str(), 0666), 0);
  errors.str("");
  EXPECT_FALSE(swm::util::plugin_file_path_trusted(good, &errors));
  EXPECT_FALSE(errors.str().empty());

  unlink(good.c_str());
  rmdir(dir.c_str());
}

TEST(plugin_trust, file_path_rejects_symlink) {
  const std::string dir = make_temp_subdir("symlink");
  const std::string target = dir + "/real.so";
  const std::string link = dir + "/swm-sched-fcfs.so";
  write_empty_file(target, 0644);
  ASSERT_EQ(symlink(target.c_str(), link.c_str()), 0);

  std::stringstream errors;
  EXPECT_FALSE(swm::util::plugin_file_path_trusted(link, &errors));
  EXPECT_FALSE(errors.str().empty());

  unlink(link.c_str());
  unlink(target.c_str());
  rmdir(dir.c_str());
}

TEST(plugin_trust, load_rejects_non_allowlisted) {
  const std::string dir = make_temp_subdir("allow");
  const std::string evil = dir + "/evil.so";
  write_empty_file(evil, 0644);

  std::stringstream errors;
  EXPECT_EQ(swm::util::load_trusted_plugin_library(evil, &errors), nullptr);
  EXPECT_NE(errors.str().find("allowlisted"), std::string::npos);

  unlink(evil.c_str());
  rmdir(dir.c_str());
}

TEST(alg, algorithm_factory_rejects_untrusted_only_plugins) {
  const std::string dir = make_temp_subdir("factory");
  // Allowlisted name but world-writable -- must not load.
  const std::string bad = dir + "/swm-sched-fcfs.so";
  write_empty_file(bad, 0666);

  swm::AlgorithmFactory factory;
  std::stringstream errors;
  EXPECT_FALSE(factory.load_plugins(dir, &errors));
  EXPECT_FALSE(errors.str().empty());

  unlink(bad.c_str());
  rmdir(dir.c_str());
}
