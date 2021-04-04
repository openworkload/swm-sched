
#pragma once

#include "defs.h"
#include "auxl/metrics.h"

namespace swm {

// Algorithm-based metrics, one instance per algorithm instance
class AlgorithmMetrics {
 public:
  AlgorithmMetrics();
  const MetricsInterface &object() const { return metrics_; }
  
  size_t scheduled_jobs() const { return (size_t)metrics_.int_value(SCHEDULED_JOBS_ID); }
  size_t update_scheduled_jobs(size_t job_count) {
    return (size_t)metrics_.update_int_value(SCHEDULED_JOBS_ID, (int32_t)job_count);
  }

 private:
  const int SCHEDULED_JOBS_ID = 1;
  util::Metrics metrics_;
};

} // swm
