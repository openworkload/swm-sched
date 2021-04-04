
#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "cli_args.h"

TEST(auxl, args_unexpected_flad) {
  swm::CliArgs args;
  const char * argv[] = { "test", "-f" };
  ASSERT_FALSE(args.init(2, argv));
}

TEST(auxl, args_inconsistency_case) {
  swm::CliArgs args;
  const char * argv[] = { "test", "-h", "-d" };
  std::stringstream stream;
  ASSERT_FALSE(args.init(3, argv, &stream));
  ASSERT_TRUE(stream.str().find("-h") != std::string::npos);
}

TEST(auxl, args_not_exist_input_file) {
  swm::CliArgs args;
  const char * argv[] = { "test", "-i", "foo.txt" };
  std::stringstream stream;
  ASSERT_FALSE(args.init(3, argv, &stream));
  ASSERT_TRUE(stream.str().find("-i") != std::string::npos);
}

TEST(auxl, args_not_exist_plugin_directory) {
  swm::CliArgs args;
  const char * argv[] = { "test", "-p", "foo" };
  std::stringstream stream;
  ASSERT_FALSE(args.init(3, argv, &stream));
  ASSERT_TRUE(stream.str().find("-p") != std::string::npos);
}

TEST(auxl, args_debug_case) {
  swm::CliArgs args;
  const char * argv[] = { "test", "-d" };
  ASSERT_TRUE(args.init(2, argv));
}

TEST(auxl, args_correct_ints) {
  swm::CliArgs args;
  const char *argv[] = { "", "--in-queue", "3", "--out-queue", "4"};
  ASSERT_TRUE(args.init(5, argv));
  size_t tmp;
  ASSERT_TRUE(args.has_in_queue_flag(&tmp));
  ASSERT_EQ(tmp, 3);
  ASSERT_TRUE(args.has_out_queue_flag(&tmp));
  ASSERT_EQ(tmp, 4);
}

TEST(auxl, args_negative_int) {
  swm::CliArgs args;
  const char *argv[] = { "", "--in-queue", "-3" };
  std::stringstream output;
  ASSERT_FALSE(args.init(3, argv, &output));
  ASSERT_TRUE(output.str().find("-3") != std::string::npos);
}

TEST(auxl, args_wrong_int) {
  swm::CliArgs args;
  const char *argv[] = { "", "--in-queue", "2.5" };
  std::stringstream output;
  ASSERT_FALSE(args.init(3, argv, &output));
  ASSERT_TRUE(output.str().find("2.5") != std::string::npos);
}

TEST(auxl, args_timeout) {
  swm::CliArgs args;
  const char *wrong_argv1[] = { "", "--timeout", "0.0" };
  ASSERT_FALSE(args.init(3, wrong_argv1));

  const char *wrong_argv2[] = { "", "--timeout", "-10" };
  ASSERT_FALSE(args.init(3, wrong_argv2));

  const char *correct_argv[] = { "", "--timeout", "3.14" };
  ASSERT_TRUE(args.init(3, correct_argv));
  double val;
  ASSERT_TRUE(args.has_timeout_flag(&val));
  ASSERT_GT(val, 3.1);
  ASSERT_LT(val, 3.2);
}
