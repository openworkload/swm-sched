
#include "plugin_defs.h"
#include "plugin_desc.h"
#include "plugin_exports.h"
#include "plugin_context.h"
#include "timetable_info.h"
#include "fcfs_implementation.h"


bool swm_create_context(swm::MetricsInterface *metrics, void **ctx, std::stringstream *) {
  // 2Taras: plugin can have its own metrics - just use injected instance "metrics"
  metrics->register_double_value(1, "the number of processed requests");
  *ctx = new PluginContext(metrics);
  return true;
}

bool swm_release_context(void *ctx, std::stringstream *) {
  delete (PluginContext *)ctx;
  return true;
}

bool swm_construct_timetable(void *,
                             const swm::SchedulingInfoInterface *sched_info,
                             swm::PluginEventsInterface *events,
                             std::shared_ptr<swm::TimetableInfoInterface> *tt_info,
                             std::stringstream *error) {
  if (sched_info->jobs().empty()) {
    tt_info->reset(new TimetableInfo());
    return true;
  }

  swm::FcfsImplementation fcfs;
  std::vector<swm::SwmTimetable> tts;
  if (!fcfs.init(sched_info, error) ||
      !fcfs.schedule(sched_info->jobs(), events, &tts, true, error)) {
    return false;
  }

  tt_info->reset(new TimetableInfo(&tts));
  return true;
}

bool swm_improve_timetable(void *,
                           const swm::TimetableInfoInterface *old_tt,
                           swm::PluginEventsInterface *,
                           std::shared_ptr<swm::TimetableInfoInterface> *new_tt,
                           std::stringstream *) {
  new_tt->reset(new TimetableInfo(old_tt));
  return true;
}

bool swm_bind_compute_unit(void *,
                           const swm::ComputeUnitInterface *cu,
                           std::stringstream *error) {
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }
  if (cu->device_type() != swm::ComputeUnitInterface::Type::Cpu) {
    *error << "Device type must be cpu";
    return false;
  }

  return true;
}

void swm_create_algorithm_descriptor(std::shared_ptr<swm::AlgorithmDescInterface> *res) {
  if (res != nullptr) {
    res->reset(new PluginDesc("swm-fcfs", "1.0", swm::ComputeUnitInterface::Cpu));
  }
}
