
#pragma once

#include "defs.h"
#include "hw/compute_unit.h"
#include "lib_binding.h"
#include "algorithm_metrics.h"

namespace swm {

//For friendship declaration.
class AlgorithmFactory;

class Algorithm {
 public:
  bool bind_to(const ComputeUnit *cu, std::stringstream *error = nullptr);
  bool create_timetable(const SchedulingInfoInterface *info,
                        PluginEventsInterface *events,
                        std::shared_ptr<swm::TimetableInfoInterface> *tt,
                        std::stringstream *error = nullptr);
  bool improve_timetable(const TimetableInfoInterface *old_tt,
                         PluginEventsInterface *events,
                         std::shared_ptr<swm::TimetableInfoInterface> *new_tt,
                         std::stringstream *error = nullptr);

  const AlgorithmDescInterface *description() const {
    return binding_->get_algorithm_descriptor();
  }
  const AlgorithmMetrics &algorithm_metrics() const { return algorithm_metrics_; }
  const MetricsInterface &plugin_metrics() const { return plugin_metrics_; }
  const std::string &plugin_location() const { return binding_->lib_location(); }

  ~Algorithm();

 private:
  Algorithm(const Algorithm &) = delete;
  Algorithm(const std::shared_ptr<util::LibBinding> &binding)
      : ctx_(nullptr), binding_(binding) { }
  void operator =(const Algorithm &) = delete;

  bool init(std::stringstream *error = nullptr);

  void *ctx_;
  std::shared_ptr<util::LibBinding> binding_;
  AlgorithmMetrics algorithm_metrics_;
  util::Metrics plugin_metrics_;
   
  friend class AlgorithmFactory;
};

}
