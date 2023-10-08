#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "auxl/lib_funcs.h"

TEST(auxl, lib_funcs_load_and_free_library) {
#if defined(WIN32)
  std::string test_lib = find_win_dir() + "\\system32\\kernel32.dll";
#else
  std::string test_lib = "/usr/libexec/sudo/sudoers.so";
#endif
  auto lib = swm::util::load_library(test_lib);
  auto success_load = lib != nullptr;
  std::string error_load = swm::util::get_library_error();
  auto success_free = swm::util::free_library(lib);
  std::string error_free = swm::util::get_library_error();
  ASSERT_TRUE(success_load) << error_load;
  ASSERT_TRUE(success_free) << error_free;
}

TEST(auxl, lib_funcs_load_and_free_x32_library) {
#if defined(WIN32)
  std::string test_lib = find_win_dir() + "\\syswow64\\kernel32.dll";
#else
  std::string test_lib = "/lib32/libutil.so.1";
#endif
  auto lib = swm::util::load_library(test_lib);
  auto success_load = lib != nullptr;
  std::string error_load = swm::util::get_library_error();
  auto success_free = swm::util::free_library(lib);
  std::string error_free = swm::util::get_library_error();
  ASSERT_FALSE(success_load) << error_load;
  ASSERT_TRUE(success_free) << error_free;
}

TEST(auxl, lib_funcs_load_and_free_unknown_lib) {
  std::string test_lib = "unknown_lib" + swm::util::get_library_extension();
  auto lib = swm::util::load_library(test_lib);
  auto success_load = lib != nullptr;
  std::string error_load = swm::util::get_library_error();
  auto success_free = swm::util::free_library(lib);
  std::string error_free = swm::util::get_library_error();
  ASSERT_FALSE(success_load) << error_load;
  ASSERT_TRUE(success_free) << error_free;
}

TEST(auxl, lib_funcs_load_func) {
#if defined(WIN32)
  std::string test_lib = find_win_dir() + "\\system32\\kernel32.dll";
  std::string test_func = "LoadLibraryA";
#else
  std::string test_lib = "/usr/libexec/sudo/sudoers.so";
  std::string test_func = "sudoers_io";
#endif
  void* lib = swm::util::load_library(test_lib);
  auto success_load = swm::util::get_library_function(lib, test_func) != nullptr;
  auto error_load = swm::util::get_library_error();
  swm::util::free_library(lib);
  ASSERT_TRUE(success_load) << error_load;
}

TEST(auxl, lib_funcs_load_unknkown_func) {
  std::string test_func = "UnknkownFunction";
#if defined(WIN32)
  std::string test_lib = find_win_dir() + "\\system32\\kernel32.dll";
#else
  std::string test_lib = "/usr/libexec/sudo/sudoers.so";
#endif
  void* lib = swm::util::load_library(test_lib);
  auto success_load = swm::util::get_library_function(lib, test_func) != nullptr;
  auto error_load = swm::util::get_library_error();
  swm::util::free_library(lib);
  ASSERT_FALSE(success_load) << error_load;
}
