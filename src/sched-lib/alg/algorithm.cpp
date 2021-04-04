
#include "algorithm.h"

namespace swm {

bool Algorithm::init(std::stringstream *error) {
  if (ctx_ != nullptr) {
    throw std::runtime_error("Algorithm::init(): algorithm has been already initialized");
  }

  return binding_->create_context(&plugin_metrics_, &ctx_, error);
}

bool Algorithm::bind_to(const ComputeUnit *cu, std::stringstream *error) {
  if (ctx_ == nullptr) {
    throw std::runtime_error("Algorithm::bind_to(): algorithm must be initizalized first");
  }

  return binding_->bind_to_compute_unit(ctx_, cu, error);
}

bool Algorithm::create_timetable(const SchedulingInfoInterface *info,
                                 PluginEventsInterface *events,
                                 std::shared_ptr<swm::TimetableInfoInterface> *tt,
                                 std::stringstream *error) {
  if (ctx_ == nullptr) {
    throw std::runtime_error(
      "Algorithm::create_timetable(): algorithm must be initizalized first");
  }
  if (info == nullptr || events == nullptr || tt == nullptr) {
    throw std::runtime_error(
      "Algorithm::create_timetable(): \"info\", \"events\" and \"tt\" cannot be equal to nullptr"
    );
  }

  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }

  bool succeeded = binding_->construct_timetable(ctx_, info, events, tt, error);
  if (succeeded) {
    if (tt->get() == nullptr) {
      *error << "plugin failed to create timetable, looks like it's an internal error";
      return false;
    }
    algorithm_metrics_.update_scheduled_jobs((*tt)->tables().size());
  }
  return succeeded;
}

bool Algorithm::improve_timetable(const TimetableInfoInterface *old_tt,
                                  PluginEventsInterface *events,
                                  std::shared_ptr<swm::TimetableInfoInterface> *new_tt,
                                  std::stringstream *error) {
  if (ctx_ == nullptr) {
    throw std::runtime_error(
      "Algorithm::improve_timetable(): algorithm must be initizalized first");
  }
  if (old_tt == nullptr /*|| events == nullptr*/ || new_tt == nullptr) {
    std::stringstream ss;
    ss << "Algorithm::improve_timetable(): ";
    ss << "\"old_tt\", \"events\" and \"new_tt\" cannot be equal to nullptr";
    throw std::runtime_error(ss.str());
  }

  return binding_->improve_timetable(ctx_, old_tt, events, new_tt, error);
}

Algorithm::~Algorithm() {
  std::stringstream error;
  if (ctx_ != nullptr && !binding_->release_context(ctx_, &error)) {
    std::cerr << "Failed to release context of the plugin "
              << binding_->lib_location().c_str() << ", the reason: "
              << error.str().c_str() << std::endl;
  }
}

} // swm
