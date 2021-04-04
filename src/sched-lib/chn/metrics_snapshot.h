
#pragma once

#include "defs.h"

#include "ifaces/metrics_interface.h"

namespace swm {
class Algorithm;
class Chain;
namespace util {

// Instantly cloned metrics for set of algorithms, single chain and global service
// These values are constant and will not be changed
class MetricsSnapshot {
 public:
  class AlgorithmMetricsSnapshot {
   public:
    AlgorithmMetricsSnapshot() = default;

    const std::string &name() const { return name_; }
    const MetricsInterface &internal_metrics() const { return *internal_metrics_.get(); }
    const MetricsInterface &external_metrics() const { return *external_metrics_.get(); }

   private:
    AlgorithmMetricsSnapshot(const Algorithm &algorithm);

    std::string name_;
    std::shared_ptr<MetricsInterface> internal_metrics_;
    std::shared_ptr<MetricsInterface> external_metrics_;

   friend class MetricsSnapshot;
  };

  MetricsSnapshot();         // empty metrics for unit tests
  MetricsSnapshot(const MetricsInterface &service_metrics, const Chain &chain);
  void operator =(const MetricsSnapshot &) = delete;

  const MetricsInterface &service_metrics() const { return *service_metrics_.get(); }
  const MetricsInterface &chain_metrics() const { return *chain_metrics_.get(); }
  const std::vector<AlgorithmMetricsSnapshot> &algorithm_metrics() const {
    return algorithm_metrics_;
  }

 private:
  std::shared_ptr<MetricsInterface> service_metrics_;
  std::shared_ptr<MetricsInterface> chain_metrics_;
  std::vector<AlgorithmMetricsSnapshot> algorithm_metrics_;
};

} // util
} // swm
