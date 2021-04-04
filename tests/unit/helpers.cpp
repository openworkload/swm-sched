
#include "helpers.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>

#if defined(WIN32)
#include <direct.h>
#else
#include <unistd.h>
#endif

// Returns directory that contains file. Not found - empty string as result
static inline std::string find_directory_with_file(const std::vector<std::string> &dirs,
                                                   const std::string &file) {
  for (size_t i = 0; i < dirs.size(); ++i) {
    if (!dirs[i].empty()) {
      std::ifstream fstr;
      fstr.open(dirs[i] + "/" + file, std::ios::binary);
      fstr.peek();
      if (fstr.good()) {
        return dirs[i];
      }
    }
  }
  return std::string();
}

// Need CMake 3.9+ to set up working directory for GTest, but we must support 3.5.1.
// So, using this stub.
std::string find_plugin_dir() {
  std::string target = "swm-sched";
  std::vector<std::string> variants;
  variants.push_back(".");
  variants.push_back("./bin");
  variants.push_back("../bin");
  variants.push_back("../../bin");

  auto dir = find_directory_with_file(variants, target);
  return dir.empty() ? "." : dir;
}

std::string find_swm_sched_dir() {
  std::string target = "CMakeLists.txt";
  std::vector<std::string> variants;
  variants.push_back(".");
  variants.push_back("..");
  variants.push_back("../..");

  auto dir = find_directory_with_file(variants, target);
  if (dir.empty()) {
    throw std::runtime_error("Failed to find swm-sched root directory");
  }
  return dir;
}

#if defined(WIN32)
std::string find_win_dir() {
  char buf[4 * 1024];
  if (GetEnvironmentVariableA("WINDIR", buf, 4 * 1024) == 0) {
    throw std::runtime_error("Failed to find Windows");
  }
  return buf;
}
#endif

std::string find_temp_dir() {
#if defined(WIN32)
  char buf[4 * 1024];
  if (GetEnvironmentVariableA("TMP", buf, 4 * 1024) == 0 &&
      GetEnvironmentVariableA("TEMP", buf, 4 * 1024) == 0) {
    throw std::runtime_error("Failed to find suitable directory for temporary objects");
  }
  return buf;
#else
  return "/tmp";
#endif
}

std::string find_converter() {
  std::string converter = "json-to-bin.escript";
  std::vector<std::string> variants;
  variants.push_back(".");
  variants.push_back("./scripts");
  variants.push_back("../scripts");
  variants.push_back("../../scripts");

  auto dir = find_directory_with_file(variants, converter);
  if (dir.empty()) {
    throw std::runtime_error("Failed to find converting script (JSON->ErlBIN)");
  }

  return dir + "/" + converter;
}

static inline bool read_to_end(const std::string &file, std::string *content) {
  if (content == nullptr) {
    return false;
  }

  std::ifstream fstr;
  fstr.open(file.c_str(), std::ios::binary);
  if (!fstr.good()) {
    return false;
  }

  std::stringstream sstr;
  sstr << fstr.rdbuf();
  *content = sstr.str();
  return true;
}

bool my_exec(const std::string &app, const std::string &args,
             std::string *out, std::string *err) {
  std::string out_file = find_temp_dir() + "/" + "my_exec.out";
  std::string err_file = find_temp_dir() + "/" + "my_exec.err";
#if defined(WIN32)
  // The logic is a bit complicated due to usage of releseable objects
  // Therefore, we will return result only after all code performed or skipped
  bool failed = false;

  // To prepare handles (files) for output and error
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;

  HANDLE h_out = nullptr;
  if (out != nullptr && !failed) {
    h_out = CreateFileA((char *)out_file.c_str(),
                        FILE_APPEND_DATA,
                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                        &sa,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (h_out == INVALID_HANDLE_VALUE) {
      failed = true;
      h_out = nullptr;
    }
  }

  HANDLE h_err = nullptr;
  if (err != nullptr && !failed) {
    h_err = CreateFileA((char *)err_file.c_str(),
                        FILE_APPEND_DATA,
                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                        &sa,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (h_err == INVALID_HANDLE_VALUE) {
      failed = true;
      h_err = nullptr;
    }
  }

  // To launch a new process
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  STARTUPINFO si;
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdInput = nullptr;
  si.hStdError = h_err;
  si.hStdOutput = h_out;

  if (!failed) {
    failed = !CreateProcessA(NULL, (char *)(app + " " + args).c_str(),
                             NULL, NULL, TRUE, CREATE_NO_WINDOW,
                             NULL, NULL, &si, &pi);
  }


  if (!failed) {
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }

  // To read output and error
  if (out != nullptr && h_out != nullptr) {
    CloseHandle(h_out);
    if (!failed) {
      failed = failed || !read_to_end(out_file, out);
    }
    remove(out_file.c_str());
  }
  if (err != nullptr && h_err != nullptr) {
    CloseHandle(h_err);
    if (!failed) {
      failed = failed || !read_to_end(err_file, err);
    }
    remove(err_file.c_str());
  }

  return !failed;
#else
  std::stringstream cmd;
  cmd << app << " " << args;
  if (out != nullptr) {
    cmd << " 1>" << out_file.c_str();
  }
  if (err != nullptr) {
    cmd << " 2>" << err_file.c_str();
  }

  if (std::system(cmd.str().c_str()) == -1) {
    return false;
  }

  if (out != nullptr) {
    if (!read_to_end(out_file, out)) {
      return false;
    }
    remove(out_file.c_str());
  }

  if (err != nullptr) {
    if (!read_to_end(err_file, err)) {
      return false;
    }
    remove(err_file.c_str());
  }

  return true;
#endif
}

#if !defined(WIN32)
#define _getcwd getcwd
#endif

bool my_getcwd(std::string *path) {
  if (path != nullptr) {
    char buf[4 * 1024];
    if (_getcwd(buf, 4 * 1024) == nullptr)
      return false;
  
    *path = buf;
    return true;
  }

  return false;
}

#if !defined(WIN32)
#define _chdir chdir
#endif

bool my_chdir(const std::string &path) {
  if (_chdir(path.c_str()) != 0)
    return false;
  return true;
}
