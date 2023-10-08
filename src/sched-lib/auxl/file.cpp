
#include "file.h"

#include <sys/stat.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#endif


namespace swm {
namespace util {

bool file_exist(const std::string &filename) {
#if defined(WIN32)
  struct _stat st;
  return _stat(filename.c_str(), &st) == 0;
#else
  struct stat st;
  return stat(filename.c_str(), &st) == 0;
#endif
}

std::string file_full_path(const std::string &filename) {
  std::string res;
#if defined(WIN32)
  int buf_size = 4096;
  char buf[4096];
  char *tmp;
  if (GetFullPathNameA(filename.c_str(), buf_size, buf, &tmp) != 0) {
    res = buf;
  }
#else
  char buf[PATH_MAX + 1];
  auto subpath = filename;
  auto started = subpath.length();
  while (realpath(subpath.data(), buf) == nullptr &&
    started != 0 && started != std::string::npos) {
    started = filename.find_last_of("/", started - 1);
    subpath = filename.substr(0, started);
  }

  switch (started)
  {
    case 0:
      res = filename;
    break;
    case std::string::npos:
      if (realpath(".", buf) != nullptr)
        res = std::string(buf) + filename;
    break;
    default:
      res = std::string(buf) + filename.substr(started);
    break;
  }
#endif
  if (res.empty()) {
    std::stringstream ss;
    ss << "File::full_path(): failed to retrieve full path of the file \"" << filename << "\"";
    throw std::runtime_error(ss.str());
  }
  return res;
}

} // util
} // swm
