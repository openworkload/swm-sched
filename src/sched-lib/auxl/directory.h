
#pragma once

#include "defs.h"

namespace swm {
namespace util {

bool directory_exist(const std::string &dirname);
std::string directory_full_path(const std::string &dirname);
void find_files(const std::string &path, const std::string &pattern, std::vector<std::string> *files);

} // util
} // swm
