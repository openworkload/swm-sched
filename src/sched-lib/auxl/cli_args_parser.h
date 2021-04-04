
#pragma once

#include <tuple>

#include "defs.h"


namespace swm {
namespace util {

class CliArgsParser {
 public:
  CliArgsParser() { }
  CliArgsParser(CliArgsParser &) = delete;
  void operator =(CliArgsParser &) = delete;

  void register_flag(const std::string &short_name, const std::string &long_name,
                     bool *flag_ptr, std::string *value_ptr = nullptr);

  bool parse(int argc, const char * const argv[], std::stringstream *output = nullptr,
             size_t *flag_num = nullptr);

 private:
  typedef std::tuple<std::string, std::string, bool *> SimpleFlag;
  typedef std::tuple<std::string, std::string, bool *, std::string *> ValuedFlag;

  bool find_simple_flag(const std::string &name, SimpleFlag *flag = nullptr);
  bool find_valued_flag(const std::string &name, ValuedFlag *flag = nullptr);

  std::vector<SimpleFlag> simple_flags_;
  std::vector<ValuedFlag> valued_flags_;
};

} // util
} // swm
