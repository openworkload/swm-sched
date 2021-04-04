#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "auxl/file.h"

TEST(auxl, file_exist_default) {
  std::string test_path;
#if defined(WIN32)
  test_path = find_win_dir() + "\\notepad.exe";
#else
  test_path = "/bin/ls";
#endif
  ASSERT_TRUE(swm::util::file_exist(test_path));
}

TEST(auxl, file_not_exist) {
  std::string test_path;
#if defined(WIN32)
  test_path = find_win_dir() + "\\notepadunknown.exe";
#else
  test_path = "/usr/bin/lswtf";
#endif
  ASSERT_FALSE(swm::util::file_exist(test_path));
}

TEST(auxl, file_full_path_default) {
  auto full_path = swm::util::file_full_path("abc");
  ASSERT_TRUE(full_path.find("swm-sched") != std::string::npos &&
              full_path.find("abc") != std::string::npos);
}
