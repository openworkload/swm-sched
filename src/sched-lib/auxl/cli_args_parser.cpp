
#include "cli_args_parser.h"

#include <algorithm>
#include <set>

namespace swm {
namespace util {

template<class FLAG_TYPE>
bool find_templated_flag(const std::vector<FLAG_TYPE> &collection,
                         const std::string &name, FLAG_TYPE *flag) {
  auto pred = [name](const FLAG_TYPE &f) -> bool {
    return std::get<0>(f) == name || std::get<1>(f) == name; };

  auto iter = std::find_if(collection.cbegin(), collection.cend(), pred);
  if (iter != collection.cend()) {
    if (flag != nullptr) *flag = *iter;
    return true;
  }

  return false;
}

bool CliArgsParser::find_simple_flag(const std::string &name, SimpleFlag *flag) {
  return find_templated_flag<SimpleFlag>(simple_flags_, name, flag);
}

bool CliArgsParser::find_valued_flag(const std::string &name, ValuedFlag *flag) {
  return find_templated_flag<ValuedFlag>(valued_flags_, name, flag);
}

void CliArgsParser::register_flag(const std::string &short_name, const std::string &long_name,
                                  bool *flag_ptr, std::string *value_ptr) {
  if (short_name.empty() && long_name.empty()) {
    throw std::runtime_error("CliArgsParser::register_flag(): both flag names cannot be empty");
  }

  if (flag_ptr == nullptr) {
    throw std::runtime_error(
      "CliArgsParser::register_flag(): flag_ptr cannot be equal to nullptr");
  }

  if (!short_name.empty() && (find_simple_flag(short_name) || find_valued_flag(short_name))) {
    throw std::runtime_error(
      "CliArgsParser::register_flag(): flag with such short name already registered");
  }
  if (!long_name.empty() && (find_simple_flag(long_name) || find_valued_flag(long_name))) {
    throw std::runtime_error(
      "CliArgsParser::register_flag(): flag with such long name already registered");
  }

  if (value_ptr == nullptr) {
    simple_flags_.push_back(std::make_tuple(short_name, long_name, flag_ptr));
  }
  else {
    valued_flags_.push_back(std::make_tuple(short_name, long_name, flag_ptr, value_ptr));
  }
}

bool CliArgsParser::parse(int argc, const char * const argv[], std::stringstream *output,
                          size_t *flag_num) {
  if (argc < 1 || argv == nullptr) {
    throw std::runtime_error("CliArgsParser::parse(): \"argc\" and/or \"argv\" have wrong values");
  }
  for (int i = 0; i < argc; i++)
    if (argv[i] == nullptr) {
      throw std::runtime_error(
        "CliArgsParser::parse(): values of \"argv[]\" cannot be equal to nullptr");
    }

  size_t flag_num_;
  if (flag_num == nullptr) {
    flag_num = &flag_num_;
  }
  *flag_num = 0;

  std::stringstream output_;
  if (output == nullptr) {
    output = &output_;
  }

  for (const auto &f : simple_flags_) *std::get<2>(f) = false;
  for (const auto &f : valued_flags_) *std::get<2>(f) = false;

  std::set<std::string> met_flags;
  for (int i = 1; i < argc; ++i) {

    std::string name = argv[i];
    *flag_num += 1;

    if (met_flags.find(name) != met_flags.end()) {
      *output << "flag \"" << name << "\" already defined";
      return false;
    }

    SimpleFlag sflag;
    if (find_simple_flag(name, &sflag)) {
      *std::get<2>(sflag) = true;
      if (!std::get<0>(sflag).empty()) {
        met_flags.insert(std::get<0>(sflag));
      }
      if (!std::get<1>(sflag).empty()) {
        met_flags.insert(std::get<1>(sflag));
      }
      continue;
    }

    ValuedFlag vflag;
    if (find_valued_flag(name, &vflag)) {
      if (i == argc - 1) {
        *output << "value for flag \"" << name << "\" expected";
        return false;
      }

      *std::get<2>(vflag) = true;
      *std::get<3>(vflag) = argv[i + 1];
      if (!std::get<0>(vflag).empty()) {
        met_flags.insert(std::get<0>(vflag));
      }
      if (!std::get<1>(vflag).empty()) {
        met_flags.insert(std::get<1>(vflag));
      }
      ++i;
      continue;
    }

    *output << "unknown flag \"" << name << "\"";
    return false;
  }
  
  return true;
}

} // util
} // swm
