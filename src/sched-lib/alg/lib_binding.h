
#pragma once

#include "../defs.h"

#include <functional>

#include "ifaces/compute_unit_interface.h"
#include "ifaces/algorithm_desc_interface.h"
#include "ifaces/scheduling_info_interface.h"
#include "ifaces/timetable_info_interface.h"
#include "ifaces/plugin_events_interface.h"
#include "ifaces/metrics_interface.h"


namespace swm {
class AlgorithmFactory;
}

extern "C" {
namespace swm {
namespace util {


class LibBinding {
 public:
  typedef bool(*CreateContextFunc)(MetricsInterface *, void **, std::stringstream *);
  typedef bool(*ReleaseContextFunc)(void *, std::stringstream *);
  typedef bool(*ConstructTimetableFunc)(void *,
                                        const SchedulingInfoInterface *,
                                        PluginEventsInterface *,
                                        std::shared_ptr<swm::TimetableInfoInterface> *,
                                        std::stringstream *);
  typedef bool(*ImproveTimetableFunc)(void *,
                                      const TimetableInfoInterface *,
                                      PluginEventsInterface *,
                                      std::shared_ptr<swm::TimetableInfoInterface> *,
                                      std::stringstream *);
  typedef bool(*BindComputeUnitFunc)(void *,
                                     const ComputeUnitInterface *,
                                     std::stringstream *);
  typedef void (*CreateAlgorithmDescFunc)(std::shared_ptr<AlgorithmDescInterface> *);

  ~LibBinding();

  bool create_context(MetricsInterface *metrics, void **ctx,
                     std::stringstream *error = nullptr) const;
  bool release_context(void *ctx, std::stringstream *error = nullptr) const;
  bool construct_timetable(void *ctx,
                           const SchedulingInfoInterface *info,
                           PluginEventsInterface *events,
                           std::shared_ptr<swm::TimetableInfoInterface> *table,
                           std::stringstream *error = nullptr) const;
  bool improve_timetable(void *ctx,
                         const TimetableInfoInterface *old_table,
                         PluginEventsInterface *events,
                         std::shared_ptr<swm::TimetableInfoInterface> *new_table,
                         std::stringstream *error = nullptr) const;
  bool bind_to_compute_unit(void *ctx, const ComputeUnitInterface *cu,
                            std::stringstream *error = nullptr) const;
  const AlgorithmDescInterface *get_algorithm_descriptor() const { return alg_desc_.get(); }
  const std::string &lib_location() const { return lib_location_; }

 private:
  LibBinding(const LibBinding &) = delete;
  LibBinding(const std::string &lib_loc,
             const std::function<std::string()> &free_lib_func,
             const std::shared_ptr<AlgorithmDescInterface> &alg_desc,
             void *create_ctx_ptr, void *release_ctx_ptr,
             void *construct_tt_ptr, void *improve_tt_ptr, void *bind_cu_ptr);
  void operator =(const LibBinding &) = delete;

  static const char *create_context_name() { return "swm_create_context"; }
  static const char *release_context_name() { return "swm_release_context"; }
  static const char *construct_timetable_name() { return "swm_construct_timetable"; };
  static const char *improve_timetable_name() { return "swm_improve_timetable"; };
  static const char *bind_compute_unit_name() { return "swm_bind_compute_unit"; };
  static const char *create_algorithm_desc_name() { return "swm_create_algorithm_descriptor"; };

  std::string lib_location_;
  std::function<std::string()> free_lib_func_;
  std::shared_ptr<AlgorithmDescInterface> alg_desc_;
  CreateContextFunc create_ctx_func_;
  ReleaseContextFunc release_ctx_func_;
  ConstructTimetableFunc construct_timetable_func_;
  ImproveTimetableFunc improve_timetable_func_;
  BindComputeUnitFunc bind_func_;

 friend class swm::AlgorithmFactory;
};

} // util
} // swm
} // extern "C"
