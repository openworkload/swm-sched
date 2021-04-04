
#pragma once

#include <gtest/gtest.h>

#include "defs.h"
#include "auxl/cli_args_parser.h"

TEST(auxl, args_parser_two_empty_flags) {
  swm::util::CliArgsParser parser;
  bool tmp;
  ASSERT_ANY_THROW(parser.register_flag("", "", &tmp));
}

TEST(auxl, args_parser_one_empty_flag) {
  swm::util::CliArgsParser parser;
  bool tmp1 = false, tmp2 = false;
  ASSERT_NO_THROW(parser.register_flag("-f", "", &tmp1));
  ASSERT_NO_THROW(parser.register_flag(std::string(), "--flag", &tmp2));
  const char *args[] = { "", "-f", "--flag" };
  ASSERT_TRUE(parser.parse(3, args));
  ASSERT_TRUE(tmp1);
  ASSERT_TRUE(tmp2);
}

TEST(auxl, args_parser_flag_redefinition) {
  swm::util::CliArgsParser parser;
  bool tmp1, tmp2;
  parser.register_flag("-h", "--help", &tmp1);
  ASSERT_ANY_THROW(parser.register_flag("-h", "--ext_help", &tmp2));
}

TEST(auxl, args_parser_capital_flag) {
  swm::util::CliArgsParser parser;
  bool tmp1, tmp2;
  parser.register_flag("-h", "--help", &tmp1);
  ASSERT_NO_THROW(parser.register_flag("-H", "--ext_help", &tmp2));
}

TEST(auxl, args_parser_flag_nullptr) {
  swm::util::CliArgsParser parser;
  ASSERT_ANY_THROW(parser.register_flag("-h", "--help", nullptr));
}

TEST(auxl, args_parser_default_simple_flag) {
  swm::util::CliArgsParser parser;
  bool tmp;
  ASSERT_NO_THROW(parser.register_flag("-h", "--help", &tmp));
}

TEST(auxl, args_parser_default_value_flag) {
  swm::util::CliArgsParser parser;
  bool tmp;
  std::string value;
  ASSERT_NO_THROW(parser.register_flag("-h", "--help", &tmp, &value));
}

TEST(auxl, args_parser_parse_argv_nullptr) {
  swm::util::CliArgsParser parser;
  bool tmp;
  parser.register_flag("-h", "--help", &tmp);
  ASSERT_ANY_THROW(parser.parse(0, nullptr));
}

TEST(auxl, args_parser_parse_double_flag) {
  swm::util::CliArgsParser parser;
  bool tmp;
  parser.register_flag("-h", "--help", &tmp);
  const char * argv[] = { "test", "-h", "-h" };
  std::stringstream stream;
  bool res = parser.parse(3, argv, &stream);
  ASSERT_FALSE(res);
  ASSERT_TRUE(stream.str().find("-h") != std::string::npos);
}

TEST(auxl, args_parser_parse_short_and_long_flag) {
  swm::util::CliArgsParser parser;
  bool tmp;
  parser.register_flag("-h", "--help", &tmp);
  const char * argv[] = { "test", "-h" , "--help" };
  std::stringstream stream;
  bool res = parser.parse(3, argv, &stream);
  ASSERT_FALSE(res);
  ASSERT_TRUE(stream.str().find("--help") != std::string::npos);
}

TEST(auxl, args_parser_parse_undefined_flag) {
  swm::util::CliArgsParser parser;
  bool tmp;
  parser.register_flag("-h", "--help", &tmp);
  const char * argv[] = { "test", "-v" };
  std::stringstream stream;
  bool res = parser.parse(2, argv, &stream);
  ASSERT_FALSE(res);
  ASSERT_TRUE(stream.str().find("-v") != std::string::npos);
}

TEST(auxl, args_parser_parse_value_expected) {
  swm::util::CliArgsParser parser;
  bool tmp;
  std::string value;
  parser.register_flag("-v", "--value", &tmp, &value);
  const char * argv[] = { "test", "-v" };
  std::stringstream stream;
  bool res = parser.parse(2, argv, &stream);
  ASSERT_FALSE(res);
  ASSERT_TRUE(stream.str().find("-v") != std::string::npos);
}

TEST(auxl, args_parser_parse_default_simple_flag) {
  swm::util::CliArgsParser parser;
  bool tmp;
  parser.register_flag("-h", "--help", &tmp);
  const char * argv[] = { "test", "-h" };
  size_t flag_num;
  bool res = parser.parse(2, argv, nullptr, &flag_num);
  ASSERT_TRUE(res);
  ASSERT_TRUE(tmp);
  ASSERT_EQ(flag_num, 1);
}

TEST(auxl, args_parser_parse_default_value_flag) {
  swm::util::CliArgsParser parser;
  bool tmp;
  std::string value;
  parser.register_flag("-v", "--value", &tmp, &value);
  const char * argv[] = { "test", "-v", "foo" };
  size_t flag_num = 0;
  bool res = parser.parse(3, argv, nullptr, &flag_num);
  ASSERT_TRUE(res);
  ASSERT_TRUE(tmp);
  ASSERT_STREQ(value.c_str(), "foo");
  ASSERT_EQ(flag_num, 1);
}
