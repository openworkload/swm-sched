
#include "cli_args.h"

#include <stdexcept>

#include "auxl/file.h"
#include "auxl/directory.h"

namespace swm {

CliArgs::CliArgs() {
  parser_.reset(new util::CliArgsParser());

  parser_->register_flag("-h", "--help", &help_flag_);
  parser_->register_flag("-d", "--debug", &debug_flag_);
  parser_->register_flag("-i", "--input", &input_flag_, &input_value_);
  parser_->register_flag("-p", "--plugins", &plugins_flag_, &plugins_value_);
  parser_->register_flag(std::string(), "--in-queue", &in_queue_flag_, &in_queue_value_);
  parser_->register_flag(std::string(), "--out-queue", &out_queue_flag_, &out_queue_value_);
  parser_->register_flag(std::string(), "--timeout", &timeout_flag_, &timeout_value_);
}

bool CliArgs::try_parse(const std::string &val, size_t *pval) {
  std::istringstream ss(val);
  int ival;
  if (!(ss >> ival) || !ss.eof() || ival < 0) { return false; }
  
  *pval = (size_t)ival;
  return true;
}

bool CliArgs::try_parse(const std::string &val, double *pval) {
  std::istringstream ss(val);
  double dval;
  if (!(ss >> dval) || !ss.eof()) { return false; }

  *pval = dval;
  return true;
}

bool CliArgs::init(int argc, const char * const argv[], std::stringstream *errors) {
  std::stringstream errors_;
  if (errors == nullptr) {
    errors = &errors_;
  }

  // step 1 - simply parsing arguments
  size_t parsed_args;
  if (!parser_->parse(argc, argv, errors, &parsed_args)) {
    return false;
  }

  // step 2 - checking for inconsistency
  if (help_flag_ && parsed_args != 1) {
    *errors << "flag \"-h\" cannot be used with any other flags";
    return false;
  }

  if (input_flag_ && !util::file_exist(input_value_)) {
    *errors << "input file defined by flag \"-i\" not exists";
    return false;
  }
  
  if (plugins_flag_ && !util::directory_exist(plugins_value_)) {
    *errors << "plug-in directory defined by flag \"-p\" not exists";
    return false;
  }

  // step 3 - parsing integer and double values
  if (in_queue_flag_ && !try_parse(in_queue_value_, &in_queue_pvalue_)) {
    *errors << "value \"" << in_queue_value_
            << "\" defined by flag \"--in-queue\" cannot be casted to size_t";
    return false;
  }

  if (out_queue_flag_ && !try_parse(out_queue_value_, &out_queue_pvalue_)) {
    *errors << "value \"" << out_queue_value_
            << "\" defined by flag \"--out-queue\" cannot be casted to size_t";
    return false;
  }

  if (timeout_flag_ && (!try_parse(timeout_value_, &timeout_pvalue_) || timeout_pvalue_ <= 0.0)) {
    *errors << "value \"" << timeout_value_
            << "\" defined by flag \"--timeout\" cannot be casted to positive double";
    return false;
  }

  return true;
}

void CliArgs::format_help_message(std::ostream *stream) {
  if (stream == nullptr) {
    return;
  }

  *stream << "Scheduler for Sky Workflow Manager" << std::endl;
  *stream << "It is an internal tool, do not launch it directly!" << std::endl;
  *stream << "Usage:" << std::endl;
  *stream << std::endl;
  *stream << "swm-sched {-h|--help}" << std::endl;
  *stream << "swm-sched [{-d|--debug}] [{-p|--plugins} <PLUGINS>] [{-i|--input} <INPUT>]" << std::endl;
  *stream << "          [--in_queue <IN_QUEUE_SIZE>] [--out_queue <IN_QUEUE_SIZE>]" << std::endl;
  *stream << "          [--timeout <TIMEOUT>]" << std::endl;
  *stream << std::endl;
  *stream << "where" << std::endl;
  *stream << "     -h, --help:" << std::endl;
  *stream << "          prints the current help message and exits" << std::endl;
  *stream << "     -d, --debug:" << std::endl;
  *stream << "          forces to print additional debug messages to log" << std::endl;
  *stream << "     -p, --plugins:" << std::endl;
  *stream << "          defines directory <PLUGINS> which plugins are stored in. Current" << std::endl;
  *stream << "          directory is used by default." << std::endl;
  *stream << "     -i, --input:" << std::endl;
  *stream << "          forces to read commands from file with name <INPUT> instead of" << std::endl;
  *stream << "          standard input" << std::endl;
  *stream << "     --in-queue:" << std::endl;
  *stream << "          specifies size of the input command queue. If it is exceeded new" << std::endl;
  *stream << "          commands will not be read from input. In pcs., the default" << std::endl;
  *stream << "          value is 4." << std::endl;
  *stream << "     --out-queue:" << std::endl;
  *stream << "          specifies size of the output command queue. If it is exceeded" << std::endl;
  *stream << "          the whole service will be suspended. In pcs., the default" << std::endl;
  *stream << "          value is 16" << std::endl;
  *stream << "     --timeout:" << std::endl;
  *stream << "          sets timeout for SWM_COMMAND_INTERRUPT, SWM_COMMAND_EXCHANGE and" << std::endl;
  *stream << "          SWM_COMMAND_COMMAND. In seconds, the default value is 10.0" << std::endl;
}

} // swm
