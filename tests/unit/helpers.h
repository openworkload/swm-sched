
#pragma once

#include "defs.h"
#include "ifaces/plugin_events_interface.h"

#if defined(WIN32)
#include <Windows.h>
#endif

// Tries to find directory with plugins. If fails, returns current
std::string find_plugin_dir();

#if defined(WIN32)
// Tries to find WINDIR. If fails, throws std::exception
std::string find_win_dir();
#endif

// Tries to find directory for temporary files. If fails, returns current
std::string find_temp_dir();

// Tries to find script that converts JSON to ErlBIN. If fails, throws std::exception
std::string find_converter();

// Tries to find root directory of swm-sched. If fails, throws std::exception.
std::string find_swm_sched_dir();

// Executes command and redirects its stdout and stderr
// Uses find_temp_dir() for temporary objects
bool my_exec(const std::string &app, const std::string &args,
             std::string *out = nullptr, std::string *err = nullptr);

// C++ wrapper for getcwd()/_getcwd()
bool my_getcwd(std::string *path);

// C++ wrapper for chdir()/_chdir()
bool my_chdir(const std::string &path);

class EmptyPluginEvents : public swm::PluginEventsInterface {
 public:
  virtual bool forced_to_interrupt() const override { return false; }
  virtual void commit_intermediate_timetable(
    const std::shared_ptr<swm::TimetableInfoInterface> &) override { }
};
