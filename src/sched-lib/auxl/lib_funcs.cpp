
#include "lib_funcs.h"

#if defined(WIN32)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace swm {
namespace util {

std::string get_library_extension() {
#if defined(WIN32)
  return ".dll";
#else
  return ".so";
#endif
}

std::string get_library_error() {
#if defined(WIN32)
  const int buf_size = 4 * 1024;
  std::unique_ptr<char[]> buf(new char[buf_size]);
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf.get(), buf_size, NULL);
  return std::string(buf.get());
#else
  auto res = dlerror();
  return res == nullptr ? "" : res;
#endif
}

void *load_library(const std::string &lib) {
#if defined(WIN32)
  return LoadLibraryA(lib.c_str());
#else
  const auto ret = dlopen(lib.c_str(), RTLD_NOW);
  if (!ret) {
    const char* error = dlerror();
    std::cerr << "Syscall dlopen failed: " << (error ? error : "Unknown error") << std::endl;
  }
  return ret;
#endif
}

void *get_library_function(void *lib, const std::string &name) {
#if defined(WIN32)
  return GetProcAddress((HMODULE)lib, name.c_str());
#else
  return dlsym(lib, name.c_str());
#endif
}

bool free_library(void *lib) {
#if defined(WIN32)
  return lib == nullptr ? true : FreeLibrary((HMODULE)lib) != 0;
#else
  return lib == nullptr ? true : dlclose(lib) == 0;
#endif
}

} // util
} // swm
