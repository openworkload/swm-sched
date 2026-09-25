
#pragma once

#include "auxl/metrics.h"
#include "defs.h"

namespace swm {

class ChainMetrics {
 public:
  ChainMetrics();
  const MetricsInterface &object() const { return metrics_; }

 private:
  util::Metrics metrics_;
};

}  // namespace swm
