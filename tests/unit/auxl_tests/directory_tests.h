#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "auxl/directory.h"

TEST(auxl, directory_exist_default) {
  std::string test_path;
#if defined(WIN32)
  test_path = find_win_dir();
#else
  test_path = "/usr/bin";
#endif
  ASSERT_TRUE(swm::util::directory_exist(test_path));
}

TEST(auxl, directory_not_exist) {
  std::string test_path;
#if defined(WIN32)
  test_path = find_win_dir() + "\\unexpected_dir";
#else
  test_path = "/usr/bin/unexpected directory";
#endif
  ASSERT_FALSE(swm::util::directory_exist(test_path));
}

TEST(auxl, directory_exist_wrong_path) {
  std::string test_path;
#if defined(WIN32)
  test_path = "\\\\";
#else
  test_path = "/\\";
#endif
  ASSERT_FALSE(swm::util::directory_exist(test_path));
}

TEST(auxl, directory_full_path) {
  auto full_test_path = swm::util::directory_full_path("abc");
  ASSERT_TRUE(full_test_path.find("abc") != std::string::npos &&
              full_test_path.find("swm-sched") != std::string::npos);
}

TEST(auxl, directory_find_files_default) {
  std::string test_path, test_pattern;
  std::vector<std::string> expected_files, find_files;
#if defined(WIN32)
  test_path = find_win_dir();
  test_pattern = "*.exe";
  expected_files.push_back(test_path + "\\explorer.exe");
  expected_files.push_back(test_path + "\\notepad.exe");
#else
  test_path = "/usr/bin";
  test_pattern = "*";
  expected_files.push_back("/usr/bin/cat");
  expected_files.push_back("/usr/bin/ls");
#endif
  swm::util::find_files(test_path, test_pattern, &find_files);
  ASSERT_FALSE(find_files.empty());
  for (auto &exp_file: expected_files) {
    ASSERT_TRUE(std::find(find_files.begin(), find_files.end(), exp_file) != find_files.end());
  }
}

