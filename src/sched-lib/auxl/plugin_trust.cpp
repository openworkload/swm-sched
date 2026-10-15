#include "plugin_trust.h"

#include "lib_funcs.h"

#include <cerrno>
#include <cstring>

#if defined(WIN32)
#include <Windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace swm {
namespace util {

namespace {

void append_error(std::stringstream *errors, const std::string &msg) {
  if (errors != nullptr) {
    *errors << msg;
  }
}

#if !defined(WIN32)

bool uid_trusted(uid_t uid) {
  return uid == 0 || uid == geteuid();
}

bool mode_not_group_other_writable(mode_t mode) {
  return (mode & (S_IWGRP | S_IWOTH)) == 0;
}

#endif

}  // namespace

std::string plugin_basename(const std::string &path) {
  const auto pos = path.find_last_of('/');
  if (pos == std::string::npos) {
    return path;
  }
  return path.substr(pos + 1);
}

bool plugin_basename_allowed(const std::string &basename) {
  for (size_t i = 0; i < kAllowedPluginBasenameCount; ++i) {
    if (basename == kAllowedPluginBasenames[i]) {
      return true;
    }
  }
  return false;
}

bool plugin_directory_trusted(const std::string &dir_path, std::stringstream *errors) {
#if defined(WIN32)
  append_error(errors, "plugin directory trust checks are not supported on Windows");
  return false;
#else
  struct stat st;
  if (lstat(dir_path.c_str(), &st) != 0) {
    append_error(errors, std::string("cannot lstat plugin directory \"") + dir_path + "\": " + std::strerror(errno));
    return false;
  }
  if (S_ISLNK(st.st_mode)) {
    append_error(errors, std::string("plugin directory must not be a symlink: \"") + dir_path + "\"");
    return false;
  }
  if (!S_ISDIR(st.st_mode)) {
    append_error(errors, std::string("plugin path is not a directory: \"") + dir_path + "\"");
    return false;
  }
  if (!uid_trusted(st.st_uid)) {
    append_error(errors,
                 std::string("plugin directory owner is not trusted (uid=") + std::to_string(st.st_uid) +
                     ", euid=" + std::to_string(geteuid()) + "): \"" + dir_path + "\"");
    return false;
  }
  if (!mode_not_group_other_writable(st.st_mode)) {
    append_error(errors, std::string("plugin directory is group- or world-writable: \"") + dir_path + "\"");
    return false;
  }
  return true;
#endif
}

bool plugin_file_trusted_fd(int fd, const std::string &path_for_errors, std::stringstream *errors) {
#if defined(WIN32)
  (void)fd;
  append_error(errors, "plugin file trust checks are not supported on Windows");
  return false;
#else
  if (fd < 0) {
    append_error(errors, "invalid plugin file descriptor");
    return false;
  }
  struct stat st;
  if (fstat(fd, &st) != 0) {
    append_error(errors,
                 std::string("cannot fstat plugin \"") + path_for_errors + "\": " + std::strerror(errno));
    return false;
  }
  if (!S_ISREG(st.st_mode)) {
    append_error(errors, std::string("plugin is not a regular file: \"") + path_for_errors + "\"");
    return false;
  }
  if (!uid_trusted(st.st_uid)) {
    append_error(errors,
                 std::string("plugin owner is not trusted (uid=") + std::to_string(st.st_uid) +
                     ", euid=" + std::to_string(geteuid()) + "): \"" + path_for_errors + "\"");
    return false;
  }
  if (!mode_not_group_other_writable(st.st_mode)) {
    append_error(errors, std::string("plugin is group- or world-writable: \"") + path_for_errors + "\"");
    return false;
  }
  return true;
#endif
}

bool plugin_file_path_trusted(const std::string &path, std::stringstream *errors) {
#if defined(WIN32)
  append_error(errors, "plugin file trust checks are not supported on Windows");
  return false;
#else
  const int fd = open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
  if (fd < 0) {
    append_error(errors, std::string("cannot open plugin \"") + path + "\": " + std::strerror(errno));
    return false;
  }
  const bool ok = plugin_file_trusted_fd(fd, path, errors);
  close(fd);
  return ok;
#endif
}

void *load_trusted_plugin_library(const std::string &path, std::stringstream *errors) {
#if defined(WIN32)
  append_error(errors, "trusted plugin load is not supported on Windows");
  return nullptr;
#else
  const std::string base = plugin_basename(path);
  if (!plugin_basename_allowed(base)) {
    append_error(errors, std::string("plugin basename not allowlisted: \"") + base + "\"");
    return nullptr;
  }

  const int fd = open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
  if (fd < 0) {
    append_error(errors, std::string("cannot open plugin \"") + path + "\": " + std::strerror(errno));
    return nullptr;
  }

  if (!plugin_file_trusted_fd(fd, path, errors)) {
    close(fd);
    return nullptr;
  }

  const std::string fd_path = std::string("/proc/self/fd/") + std::to_string(fd);
  void *lib = load_library(fd_path);
  close(fd);
  if (lib == nullptr) {
    append_error(errors, std::string("dlopen failed for trusted plugin \"") + path + "\": " + get_library_error());
  }
  return lib;
#endif
}

}  // namespace util
}  // namespace swm
