
#pragma once

#include "defs.h"
#include "auxl/time_counter.h"

namespace swm {
namespace util {

// Invariable context that can be separated from command and used for request tracking
class CommandContext {
 public:
  CommandContext() = delete;
  CommandContext(const SwmUID &id) : id_(id), timer_(new TimeCounter()) { }
  CommandContext(const CommandContext &) = delete;
  void operator =(const CommandContext &) = delete;

  const SwmUID &id() const { return id_; }
  const std::shared_ptr<TimeCounter> &timer() { return timer_; }  // can be separated from context

 private:
  SwmUID id_;
  std::shared_ptr<TimeCounter> timer_;
};

} // util
} // swm
