
#pragma once

#include "defs.h"
#include "auxl/metrics.h"

namespace swm {
namespace util {

// Global metrics counted by service
class ServiceMetrics {
 public:
  ServiceMetrics();
  ServiceMetrics(const ServiceMetrics &) = delete;
  void operator =(const ServiceMetrics &) = delete;
  const MetricsInterface &object() const { return metrics_; }

  size_t requests() const { return (size_t)metrics_.int_value(REQUESTS_ID); }
  size_t update_requests(size_t new_requests) {
    return (size_t)metrics_.update_int_value(REQUESTS_ID, (int32_t)new_requests);
  }

 private:
  const int REQUESTS_ID = 1;
  Metrics metrics_;
};

} // util
} // swm
