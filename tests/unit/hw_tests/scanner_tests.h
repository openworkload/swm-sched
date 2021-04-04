#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "hw/scanner.h"

TEST(hw, scanner_cpu_without_scan) {
  swm::Scanner sc;
  ASSERT_TRUE(sc.cpu() == nullptr);
}

TEST(hw, scanner_cpu_succeed_scan) {
  swm::Scanner sc;
  ASSERT_TRUE(sc.scan());
}

TEST(hw, scanner_cpu_rescan) {
  swm::Scanner sc;
  sc.scan();
  ASSERT_FALSE(sc.scan());
}

TEST(hw, scanner_cpu_name_test) {
  swm::Scanner sc;
  sc.scan();
  auto cpu_name = sc.cpu()->name();
  std::transform(cpu_name.begin(), cpu_name.end(), cpu_name.begin(), ::toupper);
  ASSERT_TRUE(cpu_name.find("INTEL") != std::string::npos ||
              cpu_name.find("AMD") != std::string::npos ||
              cpu_name.find("UNKNOWN CPU") != std::string::npos) << cpu_name;
}

TEST(hw, scanner_cpu_core_test) {
  swm::Scanner sc;
  sc.scan();
  auto cpu_cores = sc.cpu()->cores();
  ASSERT_TRUE(cpu_cores >= 1 && cpu_cores < 100) << cpu_cores;
}

TEST(hw, scanner_cpu_frequency_test) {
  swm::Scanner sc;
  sc.scan();
  auto cpu_freq = sc.cpu()->frequency_mhz();
  ASSERT_TRUE(cpu_freq > 100 && cpu_freq < 10000) << cpu_freq;
}