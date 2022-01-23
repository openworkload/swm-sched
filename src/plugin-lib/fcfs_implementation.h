
#pragma once

#include "plugin_defs.h"

#include <set>
#include "extended_rh.h"
#include "ifaces/plugin_events_interface.h"

namespace swm {

// Implementation of FCFS algorithm
// In future, should be universalized and placed in the separate project
//
// Example of scheduling:
//   Nodes: { #a, #b, #c }
//   Time equal jobs:  { #1 for 2 nodes, #2 for 1 node, #3 for 1 node,
//                       #4 for 3 nodes, #5 for 1 node }
//   Time   Node #a   Node #b   Node #c
//    0       #1        #1        #2
//    1       #3
//    2       #4        #4        #4
//    3       #5
//
// Supports:     checks for broken and busy nodes, resource requirements, RH-relations
// Not supports: multiple processes per node, multiple jobs per node
class FcfsImplementation {
 public:
  FcfsImplementation() = default;
  FcfsImplementation(const FcfsImplementation &) = delete;
  ~FcfsImplementation() { close(); }
  void operator =(const FcfsImplementation &) = delete;

  bool init(const SchedulingInfoInterface *sched_info, std::stringstream *error = nullptr);
  bool schedule(const std::vector<const SwmJob *> &jobs,
                PluginEventsInterface *events,
                std::vector<SwmTimetable> *tts,
                bool ignore_priorities,
                std::stringstream *error = nullptr);
  void close();

 private:
  // The reference to instance of SwmNode that is bounded with time field
  // Equal to std::pair but simplifies the understanding of scheduling code
  class NodeRef {
   public:
    NodeRef() : node_(nullptr), when_free_(0) { }
    NodeRef(const SwmNode *node) : node_(node), when_free_(0) { }
    NodeRef(const NodeRef &) = default;

    const SwmNode *node() const { return node_; }
    uint64_t &when_free() { return when_free_; }
    const uint64_t &when_free() const { return when_free_; }

   private:
    const SwmNode *node_;
    uint64_t when_free_;  // from the start of scheduling
  };

  // Bundle of the original job, timetable's index and scheduled nodes
  // Such structure is used for gang alignment, to avoid excessive find operations
  class JobRef {
   public:
    JobRef() : tt_(nullptr), job_(nullptr) { }
    JobRef(SwmTimetable *tt, const SwmJob *job) : tt_(tt), job_(job) { }
    JobRef(const JobRef &) = default;

    SwmTimetable *tt() const { return tt_; }
    const SwmJob *job() const { return job_; }
    std::vector<NodeRef *> &nodes() { return nodes_; }

   private:
    SwmTimetable *tt_;
    const SwmJob *job_;
    std::vector<NodeRef *> nodes_;
  };

  void align_jobs(std::vector<JobRef> *jobs,
                  std::unordered_map<std::string, uint64_t> *jobs_to_endtimes,
                  uint64_t start_time);
  bool is_dynamic_request(const std::string &req_name) const;
  bool does_node_fit_request(const std::vector<SwmResource> &requests,
                             const std::vector<SwmResource> &resources,
                             std::stringstream *error) const;
  bool schedule_single_job(const SwmJob *job,
                           uint64_t start_time_threshold,
                           std::set<std::string> *busy_nodes,
                           SwmTimetable *tt,
                           JobRef *job_ref = nullptr,
                           std::stringstream *error = nullptr);

  bool is_node_owned_by_other_job(const SwmJob &job,
                                  const std::vector<SwmResource> &resources) const;

  // Active nodes that are always sorted by "when_free" time, distributed by clusters
  std::unordered_map<std::string, std::vector<NodeRef *> > nodes_per_cluster_;
  ExtendedRH rh_;
};

} // swm
