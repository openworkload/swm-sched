
#pragma once

#include <tuple>
#include <ostream>

#include "defs.h"
#include "auxl/cli_args_parser.h"

namespace swm {

class CliArgs {
 public:
  CliArgs();
  CliArgs(const CliArgs &) = delete;
  void operator =(const CliArgs &) = delete;

  bool init(int argc, const char * const argv[], std::stringstream *errors = nullptr);

  bool has_help_flag() const { return help_flag_; }
  bool has_debug_flag() const { return debug_flag_; }
  
  bool has_input_flag(std::string *value = nullptr) const {
    if (value != nullptr) { *value = input_value_; }
    return input_flag_;
  }
  
  bool has_plugins_flag(std::string *value = nullptr) const {
    if (value != nullptr) { *value = plugins_value_; }
    return plugins_flag_;
  }

  bool has_in_queue_flag(size_t *value = nullptr) const {
    if (value != nullptr) { *value = in_queue_pvalue_; }
    return in_queue_flag_;
  }

  bool has_out_queue_flag(size_t *value = nullptr) const {
    if (value != nullptr) { *value = out_queue_pvalue_; }
    return out_queue_flag_;
  }

  bool has_timeout_flag(double *value = nullptr) const {
    if (value != nullptr) { *value = timeout_pvalue_; }
    return timeout_flag_;
  }

  static void format_help_message(std::ostream *stream);
 
 private:
  bool try_parse(const std::string &val, size_t *pval);
  bool try_parse(const std::string &val, double *pval);

  std::unique_ptr<swm::util::CliArgsParser> parser_;
  bool help_flag_;
  bool debug_flag_;
  bool input_flag_; std::string input_value_;
  bool plugins_flag_; std::string plugins_value_;
  bool in_queue_flag_; std::string in_queue_value_; size_t in_queue_pvalue_;
  bool out_queue_flag_; std::string out_queue_value_; size_t out_queue_pvalue_;
  bool timeout_flag_; std::string timeout_value_; double timeout_pvalue_;
};

} // swm
