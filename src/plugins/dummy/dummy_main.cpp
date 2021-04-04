
#include "plugin_defs.h"
#include "plugin_desc.h"
#include "plugin_exports.h"
#include "plugin_context.h"
#include "timetable_info.h"


bool swm_create_context(swm::MetricsInterface *metrics, void **ctx, std::stringstream *) {
  *ctx = new PluginContext(metrics);
  return true;
}

bool swm_release_context(void *ctx, std::stringstream *) {
  delete (PluginContext *)ctx;
  return true;
}

bool swm_construct_timetable(void *,
                             const swm::SchedulingInfoInterface *sched_info,
                             swm::PluginEventsInterface *ev,
                             std::shared_ptr<swm::TimetableInfoInterface> *tt_info,
                             std::stringstream *) {
  auto jobs = sched_info->jobs();
  auto nodes = sched_info->nodes();

  // Our use case: assign single job to single node
  // Depends on job id, result will be returned immediately or hold till interruption event
  if (jobs.size() == 1 && nodes.size() == 1) {
    std::vector<swm::SwmTimetable> tables;
    swm::SwmTimetable timetable;

    timetable.set_start_time(0);
    timetable.set_job_id(jobs[0]->get_id());
    timetable.set_job_nodes({ nodes[0]->get_id() });
    
    tables.push_back(timetable);
    tt_info->reset(new TimetableInfo(&tables));
    ev->commit_intermediate_timetable(*tt_info);

    if (jobs[0]->get_id() == "hold_on") {
      while (!ev->forced_to_interrupt()) {
        std::this_thread::yield();
      }

      return false;
    }
    else {
      return true;
    }
  }
  else {
    return false;
  }
}

bool swm_improve_timetable(void *,
                           const swm::TimetableInfoInterface *old_tt,
                           swm::PluginEventsInterface *ev,
                           std::shared_ptr<swm::TimetableInfoInterface> *new_tt,
                           std::stringstream *) {
  // The same idea as in swm_construct_timetable()
  if (old_tt->tables().size() != 1) {
    return false;
  }

  //std::vector<swm::SwmTimetable> tts(old_tt->tables());
  new_tt->reset(new TimetableInfo(old_tt));
  ev->commit_intermediate_timetable(*new_tt);

  if (old_tt->tables()[0]->get_job_id() == "hold_on") {
    while (!ev->forced_to_interrupt()) {
      std::this_thread::yield();
    }
    return false;
  }
  else {
    return true;
  }
}

bool swm_bind_compute_unit(void *,
                           const swm::ComputeUnitInterface *,
                           std::stringstream *) {
  return true;
}

void swm_create_algorithm_descriptor(std::shared_ptr<swm::AlgorithmDescInterface> *res) {
  if (res != nullptr) {
    res->reset(new PluginDesc("swm-dummy", "1.0", swm::ComputeUnitInterface::Cpu));
  }
}
