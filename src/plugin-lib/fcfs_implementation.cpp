
#include "fcfs_implementation.h"

#include <set>
#include <algorithm>
#include "timetable_info.h"

namespace swm {

bool FcfsImplementation::init(const swm::SchedulingInfoInterface *sched_info,
                              std::stringstream *error) {
  if (!rh_.init(sched_info, error)) {
    return false;   // message was already constructed by extended rh
  }

  for (const auto cluster : sched_info->clusters()) {
    nodes_per_cluster_.insert(std::make_pair(cluster->get_id(), std::vector<NodeRef *>()));
  }

  const auto nodes = sched_info->nodes();
  const swm::SwmCluster *ref;
  for (const auto node : nodes) {
    if (node->get_state_power() == "up" &&
        node->get_state_alloc() == "idle" &&
        (ref = rh_.node_to_cluster(node))->get_state() == "up" &&
        rh_.node_to_part(node)->get_state() == "up") {
      nodes_per_cluster_[ref->get_id()].emplace_back(new NodeRef(node));
    }
  }

  return true;
}

bool FcfsImplementation::schedule(const std::vector<const swm::SwmJob *> &jobs,
                                  swm::PluginEventsInterface *events,
                                  std::vector<swm::SwmTimetable> *tts,
                                  bool ignore_priorities,
                                  std::stringstream *error) {
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }
  if (events == nullptr || tts == nullptr) {
    throw std::runtime_error(
      "FcfsImplementation::schedule(): \"events\" and \"tts\" cannot be equal to nullptr");
  }

  // Sort jobs according to their priorities and gang id
  const std::vector<const swm::SwmJob *> *jobs_ref = &jobs;
  std::vector<const swm::SwmJob *> sorted_jobs;
  if (!ignore_priorities) {
    sorted_jobs = jobs;
    jobs_ref = &sorted_jobs;

    std::sort(sorted_jobs.begin(), sorted_jobs.end(),
              [](const swm::SwmJob *j1, const swm::SwmJob *j2) -> bool {
      return j1->get_priority() != j2->get_priority()
              ? j1->get_priority() > j2->get_priority()
              : j1->get_gang_id()  < j2->get_gang_id();
    });
  }

  // Process all queued jobs
  std::unordered_map<std::string, uint64_t> jobs_to_endtimes;
  std::string gang_id;
  std::set<std::string> known_gang_ids;
  std::set<std::string> gang_nodes;
  std::vector<JobRef> gang_jobs;
  uint64_t gang_start_time = 0;
  tts->resize(jobs_ref->size());
  size_t tt_number = 0;
  for (size_t i = 0; i < jobs_ref->size(); i++) {
    if (events->forced_to_interrupt()) {
      return false;
    }

    const auto job = (*jobs_ref)[i];
    if (job->get_state() == "Q") {
      auto &tt = (*tts)[tt_number];
      tt.set_job_id(job->get_id());

      // Check the dependencies. All dependent jobs must already be scheduled.
      uint64_t start_time_threshold = 0;
      {
        bool has_deps = false;
        for (const auto &dep : job->get_deps()) {
          auto iter = jobs_to_endtimes.find(dep.x2);
          if (iter == jobs_to_endtimes.end()) {
            //std::cerr << "FCFS limitation: job #\"" << job->get_id() << "\" depends on "
            //  << "job #\"" << dep.x2 << "\" and must be scheduled after it";
            has_deps = true;
            break;
          }
          start_time_threshold = std::max(start_time_threshold, iter->second);
        }
        if (has_deps) { continue; }
      }

      // Check the gang id
      if (job->get_gang_id() != gang_id) {
        // Check that the new gang id is unique
        if (!job->get_gang_id().empty()) {
          auto gang_iter = known_gang_ids.find(job->get_gang_id());
          if (gang_iter != known_gang_ids.end()) {
            //std::cerr << "FCFS limitation: jobs from the gang #\"" << job->get_gang_id() << "\""
            //          << " must be placed in a row";
            continue;
          }
          known_gang_ids.insert(job->get_gang_id());
        }

        // Align jobs from the previous gang
        if (!gang_id.empty()) {
          align_jobs(&gang_jobs, &jobs_to_endtimes, gang_start_time);
        }

        // Start new gang, reset the auxiliary variables
        gang_nodes.clear();
        gang_jobs.clear();
        gang_start_time = 0;
        gang_id = job->get_gang_id();
      }
      else if (job->get_gang_id().empty()) {
        // No real gang detected, only empty identifiers. Need to reset auxiliary variables
        gang_nodes.clear();
        gang_jobs.clear();
        gang_start_time = 0;
      }

      // Schedule job and register its end time
      JobRef jr;
      if (!schedule_single_job(job, start_time_threshold, &gang_nodes, &tt, &jr, error)) {
        std::cerr << "Can't schedule job #\"" << job->get_id() << "\": " << error->str() << std::endl;
        continue;
      }

      jobs_to_endtimes[job->get_id()] = tt.get_start_time() + job->get_duration();
      gang_start_time = std::max(gang_start_time, tt.get_start_time());
      gang_jobs.emplace_back(jr);
      tt_number += 1;
    }
  }

  // Do not forget to align jobs from the last gang and resize tts
  if (!gang_id.empty()) {
    align_jobs(&gang_jobs, &jobs_to_endtimes, gang_start_time);
  }
  tts->resize(tt_number);
  return true;
}

void FcfsImplementation::close() {
  for (auto cluster : nodes_per_cluster_) {
    for (auto pn : cluster.second) {
      delete pn;
    }
  }
}

void FcfsImplementation::align_jobs(std::vector<JobRef> *jobs,
                                    std::unordered_map<std::string, uint64_t> *jobs_to_endtimes,
                                    uint64_t start_time) {
  // Shift all timetables, update node references, collect clusters that need to be resorted
  std::set<std::string> clusters;
  for (auto &jr : (*jobs)) {
    jr.tt()->set_start_time(start_time);
    for (auto nr : jr.nodes()) {
      nr->when_free() = start_time + jr.job()->get_duration();
    }
    clusters.insert(jr.job()->get_cluster_id());
    (*jobs_to_endtimes)[jr.job()->get_id()] = start_time + jr.job()->get_duration();
  }

  // Update object's collection "nodes_per_cluster_"
  auto comp = [](const NodeRef *r1, const NodeRef *r2) -> bool {
    return r2->when_free() > r1->when_free();
  };
  for (auto cluster_id : clusters) {
    auto iter = nodes_per_cluster_.find(cluster_id);
    if (iter == nodes_per_cluster_.end()) {
      throw std::runtime_error(
        "FcfsImplementation::align_timetables(): internal error, no such cluster");
    }
    std::sort(iter->second.begin(), iter->second.end(), comp);
  }
}

bool FcfsImplementation::is_node_owned_by_other_job(const swm::SwmJob &job,
                                                    const std::vector<swm::SwmResource> &resources) const {
  const auto job_id = job.get_id();
  auto iter_res = std::find_if(resources.begin(), resources.end(),
                              [job_id](const SwmResource &res) -> bool {
    if (res.get_name() != "job") {
      return false;
    }
    const std::vector<SwmTupleAtomEterm> props = res.get_properties();
    auto iter_id = std::find_if(props.begin(), props.end(),
                                [](const SwmTupleAtomEterm &prop) -> bool {
      return prop.x1 == "id";
    });
    if (iter_id == props.end()) {
      return false;
    }

    std::string prop_id;
    if (eterm_to_str(iter_id->x2, prop_id)) {
      return false;
    }
    return res.get_name() == "job" && prop_id != job_id;
  });
  return iter_res != resources.end();
}

// Check if request does not block node to be selected for the job
bool FcfsImplementation::is_dynamic_request(const std::string &req_name) const {
  static const std::vector<std::string> dyn_req_names {"node", "image", "ports"};
  return std::find(dyn_req_names.begin(), dyn_req_names.end(), req_name) != dyn_req_names.end();
}

bool FcfsImplementation::schedule_single_job(const swm::SwmJob *job,
                                             uint64_t start_time_threshold,
                                             std::set<std::string> *busy_nodes,
                                             swm::SwmTimetable *tt,
                                             JobRef *job_ref,
                                             std::stringstream *error) {
  // Step 0 - some preparations
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }

  // Step 1 - get requests and extract the total number of nodes
  const auto &requests = job->get_request();
  auto pred = [](const swm::SwmResource &res) -> bool {
    return res.get_name() == "node";
  };
  auto node_num_iter = std::find_if(requests.begin(), requests.end(), pred);
  if (node_num_iter == requests.end()) {
    *error << "job has not defined resource \"node\", unable to allocate it";
    return false;
  }
  auto node_num = node_num_iter->get_count();
  if (node_num < 1) {
    *error << "job's resource \"node\" must be greater or equal to 1";
    return false;
  }
  auto nodes_iter = nodes_per_cluster_.find(job->get_cluster_id());
  if (nodes_iter == nodes_per_cluster_.end()) {
    *error << "there is no cluster with such id (#" << job->get_cluster_id() << ")";
    return false;
  }
  auto &nodes = nodes_iter->second;

  // Step 2 - select nodes that satisfy the requirements
  const auto job_nodes_vec = job->get_nodes();
  std::set<std::string> node_ids_set(job_nodes_vec.begin(), job_nodes_vec.end());
  std::vector<NodeRef *> selected_nodes;

  for (auto &nr : nodes) {
    const auto node_id = nr->node()->get_id();

    // Exclude nodes from the busy list
    auto busy_iter = busy_nodes->find(node_id);
    if (busy_iter != busy_nodes->end()) {
      continue;
    }

    // Check pre-set node ids
    if (job_nodes_vec.size()) {
      const auto found = node_ids_set.find(node_id);
      if (found == node_ids_set.end()) {
        continue;
      }
      node_ids_set.erase(found);
    }

    // Check resources and requirements
    bool fits = true;
    const auto &resources = nr->node()->get_resources();
    for (const auto &req : requests) {
      if (!is_dynamic_request(req.get_name())) {
        auto iter = std::find_if(resources.begin(), resources.end(), [req](const swm::SwmResource &res) -> bool {
          return req.get_name() == res.get_name() && req.get_count() <= res.get_count();
        });

        if (iter == resources.end()) {
          fits = false;
          break;
        } // if
      } // if
    } // for

    if (is_node_owned_by_other_job(*job, resources)) {
      continue;
    }

    if (fits) {
      selected_nodes.emplace_back(nr);
    } // if
  } // for


  // Step 3 - the first "node_num" nodes are preferred to use
  //          because the vector are sorted by "when_free" times
  //          But we will check the following nodes as well,
  //          probably, they are placed in the better partition
  if (node_num > selected_nodes.size()) {
    *error << "not enough nodes";
    return false;
  }
  auto ext_node_num = node_num;
  while (ext_node_num < selected_nodes.size() &&
         selected_nodes[ext_node_num]->when_free() == selected_nodes[node_num - 1]->when_free()) {
    ext_node_num += 1;
  }
  selected_nodes.resize(ext_node_num);

  // Step 4 - if we have a choice, select the nodes from the same partition
  if (selected_nodes.size() > node_num) {
    // We have to separate references of the selected nodes by partitions
    std::unordered_map<const swm::SwmPartition *, std::vector<NodeRef *> > parts_to_nodes;
    for (auto &nr : selected_nodes) {
      parts_to_nodes[rh_.node_to_part(nr->node())].emplace_back(nr);
    }

    // Now we need to sort groups with nodes by their sizes to select the largest ones
    std::vector<const std::vector<NodeRef *> *> refs;
    refs.reserve(parts_to_nodes.size());
    for (const auto &rec : parts_to_nodes) {
      refs.emplace_back(&rec.second);
    }
    std::sort(refs.begin(), refs.end(), [](const std::vector<NodeRef *> *v1, 
                                           const std::vector<NodeRef *> *v2)
                                        -> bool {
      return v2->size() < v1->size();
    });

    // Finally, refill vector "selected_nodes" by nodes
    // that are placed in the most populated partitions
    selected_nodes.clear();
    for (const auto part : refs) {
      selected_nodes.insert(selected_nodes.end(), part->begin(), part->end());
      if (selected_nodes.size() >= node_num) {
        break;
      }
    }
    selected_nodes.resize(node_num);
  }

  // Step 5 - done! We need to create and fill the timetable by node's identifiers
  //          Also we need to update/resort vector "nodes" and extend collection "busy_nodes"
  auto first_free_node = std::min_element(selected_nodes.begin(), selected_nodes.end(),
                                          [](const NodeRef *v1, const NodeRef *v2) -> bool {
    return v1->when_free() > v2->when_free();
  });
  uint64_t start_time = std::max(start_time_threshold, (*first_free_node)->when_free());
  tt->set_job_id(job->get_id());
  tt->set_start_time(start_time);
  std::vector<std::string> node_ids;
  node_ids.reserve(selected_nodes.size());
  for (const auto pn : selected_nodes) {
    pn->when_free() = start_time + job->get_duration();
    node_ids.push_back(pn->node()->get_id());
    busy_nodes->insert(pn->node()->get_id());
  }
  tt->set_job_nodes(node_ids);
  if (job_ref != nullptr) {
    *job_ref = JobRef(tt, job);
    job_ref->nodes() = std::move(selected_nodes);
  }

  std::sort(nodes.begin(), nodes.end(), [](const NodeRef *r1, const NodeRef *r2) -> bool {
    return r2->when_free() > r1->when_free();
  });

  return true;
}

} // swm
