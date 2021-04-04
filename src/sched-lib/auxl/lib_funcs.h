
#pragma once

#include "defs.h"

namespace swm {
namespace util {

std::string get_library_extension();
std::string get_library_error();
void *load_library(const std::string &lib);
void *get_library_function(void *lib, const std::string &name);
bool free_library(void *lib);

} // util
} // swm
