#pragma once

#include "defs.h"

#include <cstddef>

namespace swm {
namespace util {

// Explicit allowlist of scheduler plugin basenames (CMake OUTPUT_NAME).
constexpr const char *kAllowedPluginBasenames[] = {
    "swm-sched-dummy.so",
    "swm-sched-fcfs.so",
    "swm-sched-fair-sharing.so",
};

constexpr size_t kAllowedPluginBasenameCount = sizeof(kAllowedPluginBasenames) / sizeof(kAllowedPluginBasenames[0]);

// Last path component (after final '/').
std::string plugin_basename(const std::string &path);

bool plugin_basename_allowed(const std::string &basename);

// Directory must exist, be owned by root or euid, and not group/world-writable.
bool plugin_directory_trusted(const std::string &dir_path, std::stringstream *errors = nullptr);

// fstat-based check for an already-opened plugin fd (regular file, owner, mode).
bool plugin_file_trusted_fd(int fd, const std::string &path_for_errors, std::stringstream *errors = nullptr);

// open(O_NOFOLLOW) + fstat trust check; does not dlopen. Closes the fd.
bool plugin_file_path_trusted(const std::string &path, std::stringstream *errors = nullptr);

// Allowlist + trusted open + dlopen("/proc/self/fd/N"). Returns nullptr on reject/failure.
// On success, *kept_fd holds the open descriptor that must stay open until after dlclose
// (closing early reuses /proc/self/fd/N names and conflates distinct plugins).
void *load_trusted_plugin_library(const std::string &path, std::stringstream *errors = nullptr, int *kept_fd = nullptr);

}  // namespace util
}  // namespace swm
