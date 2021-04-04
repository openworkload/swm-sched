
#pragma once

#include "plugin_defs.h"

#if defined(WIN32)
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#else
#define EXPORT
#define IMPORT
#endif

extern "C" {
  EXPORT bool swm_create_context(swm::MetricsInterface *, void **, std::stringstream *);
  EXPORT bool swm_release_context(void *, std::stringstream *);
  EXPORT bool swm_construct_timetable(void *,
                                      const swm::SchedulingInfoInterface *,
                                      swm::PluginEventsInterface *,
                                      std::shared_ptr<swm::TimetableInfoInterface> *,
                                      std::stringstream *);
  EXPORT bool swm_improve_timetable(void *,
                                    const swm::TimetableInfoInterface *,
                                    swm::PluginEventsInterface *,
                                    std::shared_ptr<swm::TimetableInfoInterface> *,
                                    std::stringstream *);
  EXPORT bool swm_bind_compute_unit(void *,
                                    const swm::ComputeUnitInterface *,
                                    std::stringstream *);
  EXPORT void swm_create_algorithm_descriptor(std::shared_ptr<swm::AlgorithmDescInterface> *res);
} // extern "C"

