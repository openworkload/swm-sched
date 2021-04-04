
#include "metrics_snapshot.h"

#include "chain.h"

namespace swm {
namespace util {

MetricsSnapshot::AlgorithmMetricsSnapshot::AlgorithmMetricsSnapshot(const Algorithm &algorithm)
    : name_(algorithm.description()->family_id()),
      internal_metrics_(algorithm.algorithm_metrics().object().clone()),
      external_metrics_(algorithm.plugin_metrics().clone()) {
}

MetricsSnapshot::MetricsSnapshot() {
  Metrics empty_metrics;
  service_metrics_ = empty_metrics.clone();
  chain_metrics_ = empty_metrics.clone();
}

MetricsSnapshot::MetricsSnapshot(const MetricsInterface &service_metrics, const Chain &chain)
    : service_metrics_(service_metrics.clone()),
      chain_metrics_(chain.metrics().object().clone()) {

  const auto &algs = chain.algorithms();
  algorithm_metrics_.resize(algs.size());
  for (size_t i = 0; i < algs.size(); ++i) {
    algorithm_metrics_.emplace_back(AlgorithmMetricsSnapshot(*algs[i]));
  }
}

} // util
} // swm
