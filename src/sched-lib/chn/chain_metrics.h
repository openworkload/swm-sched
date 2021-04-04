
#pragma once

#include "defs.h"
#include "auxl/metrics.h"

namespace swm {

class ChainMetrics {
 public:
  ChainMetrics();
  const MetricsInterface &object() const { return metrics_; }

 private:
  util::Metrics metrics_;
};

} // swm
