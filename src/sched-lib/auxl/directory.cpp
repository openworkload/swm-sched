
#include "directory.h"

#if defined(WIN32)
#include <Windows.h>
#else
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ftw.h>
#include <dirent.h>
#include <fnmatch.h>
#endif

namespace swm {
namespace util {

bool directory_exist(const std::string &dirname) {
#if defined(WIN32)
  DWORD res = GetFileAttributesA(dirname.c_str());
  if (res == INVALID_FILE_ATTRIBUTES) {
    return false;
  }

  if (res & FILE_ATTRIBUTE_DIRECTORY) {
    return true;
  }
#else
  struct stat st;
  if (stat(dirname.c_str(), &st) != 0)
    return false;

  if (S_ISDIR(st.st_mode))
    return true;
#endif
  return false;
}

std::string directory_full_path(const std::string &dirname) {
  std::string res;
#if defined(WIN32)
  int bufSize = 4096;
  char buf[4096];
  char *tmp;
  if (GetFullPathNameA(dirname.c_str(), bufSize, buf, &tmp) != 0) {
    res = buf;
  }
#else
  char buf[PATH_MAX + 1];
  auto subpath = dirname;
  auto started = subpath.length();
  while (realpath(subpath.data(), buf) == nullptr && 
    started != 0 && started != std::string::npos) {
    started = dirname.find_last_of("/", started - 1);
    subpath = dirname.substr(0, started);
  }

  switch (started)
  {
    case 0:
      res = dirname;
    break;
    case std::string::npos:
      if (realpath(".", buf) != nullptr) {
        res = std::string(buf) + dirname;
      }
    break;
    default:
      res = std::string(buf) + dirname.substr(started);
    break;
  }

#endif
  if (res.empty()) {
    std::stringstream ss;
    ss << "Directory::full_path(): failed to retrieve full path of the directory \"";
    ss << dirname << "\"";
    throw std::runtime_error(ss.str());
  }
  return res;
}

#if defined(WIN32)
#define PATH_SEPARATOR '\\'
#else 
#define PATH_SEPARATOR '/' 
#endif

void find_files(const std::string &path, const std::string &pattern,
                std::vector<std::string> *files) {
  if (files == nullptr) {
    throw std::runtime_error("Directory::find_files(): pointer \"files\" is equal to nullptr");
  }

  if (!directory_exist(path)) {
    throw std::runtime_error("Directory::find_files(): directory not found");
  }

  std::string full_path = directory_full_path(path);
  if (full_path.back() != PATH_SEPARATOR) {
    full_path.push_back(PATH_SEPARATOR);
  }

#if defined(WIN32)
  WIN32_FIND_DATA find_file_data;
  HANDLE h_find;
  std::string str = full_path + pattern;

  if (str.length() > MAX_PATH - 1) {
    throw std::runtime_error("Directory::find_files(): path is too long");
  }

  h_find = FindFirstFile(str.c_str(), &find_file_data);
  if (h_find == INVALID_HANDLE_VALUE) {
    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
      return;
    }

    throw std::runtime_error("Directory::find_files(): invalid path or pattern");
  }

  bool found = true;
  while (found) {
    if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      files->push_back(full_path + find_file_data.cFileName);
    }
    found = FindNextFile(h_find, &find_file_data) != 0;
  }

  FindClose(h_find);
#else
  DIR *dp;
  struct dirent *dirp;
  if ((dp = opendir(full_path.c_str())) == NULL) {
    throw std::runtime_error("Directory::find_files(): cannot open directory stream");
  }
  
  struct stat st;
  while ((dirp = readdir(dp)) != NULL) {
    std::string full_name = full_path + dirp->d_name;
    if (stat(full_name.data(), &st) == 0 && S_ISREG(st.st_mode)) {
      if (fnmatch(pattern.c_str(), dirp->d_name, FNM_PATHNAME) == 0) {
        std::string filename = dirp->d_name;
        if (filename != "." && filename != "..") {
          files->push_back(full_name);
        } // if
      } // if
    } // if
  } // while

  closedir(dp);
#endif
}

} // util
} // swm
