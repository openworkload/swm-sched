
#pragma once

#include "defs.h"

#include "wm_timetable.h"

extern "C"
namespace swm {

class TimetableInfoInterface {
 public:
  virtual ~TimetableInfoInterface() { }
  virtual const std::vector<const swm::SwmTimetable *> &tables() const = 0;
  virtual bool empty() const = 0;
};

} // swm
// extern "C"
