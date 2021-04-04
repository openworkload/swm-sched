
#pragma once

#include "defs.h"
#include "timetable_info_interface.h"

extern "C" {
namespace swm {

class PluginEventsInterface {
 public:
  virtual ~PluginEventsInterface() { }
  virtual bool forced_to_interrupt() const = 0;
  virtual void commit_intermediate_timetable(const std::shared_ptr<TimetableInfoInterface> &tt) = 0;
};

} // extern "C"
} // swm
