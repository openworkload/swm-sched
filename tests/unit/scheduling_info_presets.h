
#pragma once

#include "ctrl/scheduling_info.h"
#include "test_defs.h"

// A set of simple presets that used in tests
class SchedulingInfoPresets {
 public:
  // Single job must be scheduled to single node
  static std::shared_ptr<swm::SchedulingInfoInterface> one_node_one_job(const std::string &job_id);
};
