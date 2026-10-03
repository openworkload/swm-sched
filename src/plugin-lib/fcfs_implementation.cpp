
#include "fcfs_implementation.h"

#include "timetable_info.h"
#include "wm_io.h"

#include <algorithm>
#include <cinttypes>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace swm;

bool FcfsImplementation::init(const SchedulingInfoInterface *sched_info, std::stringstream *error) {
  if (!rh_.init(sched_info, error)) {
    return false;  // message was already constructed by extended rh
  }

  for (const auto cluster : sched_info->clusters()) {
    nodes_per_cluster_.insert(std::make_pair(cluster->get_id(), std::vector<NodeRef *>()));
  }

  const auto nodes = sched_info->nodes();
  const SwmCluster *cluster;
  size_t schedulable = 0;
  size_t templates = 0;
  for (const auto node : nodes) {
    cluster = rh_.node_to_cluster(node);
    const bool is_up = node->get_state_power() == "up" && node->get_state_alloc() == "idle" &&
                       cluster->get_state() == "up" && rh_.node_to_part(node)->get_state() == "up";
    if (node->get_is_template() == "true" || is_up) {
      nodes_per_cluster_[cluster->get_id()].emplace_back(new NodeRef(node));
      ++schedulable;
      if (node->get_is_template() == "true") {
        ++templates;
      }
    }
  }

  swm_logi("FCFS init: jobs=%zu nodes_total=%zu schedulable=%zu templates=%zu clusters=%zu",
           sched_info->jobs().size(),
           nodes.size(),
           schedulable,
           templates,
           sched_info->clusters().size());
  return true;
}

bool FcfsImplementation::schedule(const std::vector<const SwmJob *> &jobs,
                                  PluginEventsInterface *events,
                                  std::vector<SwmTimetable> *tts,
                                  bool ignore_priorities,
                                  std::stringstream *error) {
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }
  if (events == nullptr || tts == nullptr) {
    throw std::runtime_error("FcfsImplementation::schedule(): \"events\" and \"tts\" cannot be equal to nullptr");
  }

  // Sort jobs according to their priorities and gang id
  const std::vector<const SwmJob *> *jobs_ref = &jobs;
  std::vector<const SwmJob *> sorted_jobs;
  if (!ignore_priorities) {
    sorted_jobs = jobs;
    jobs_ref = &sorted_jobs;

    std::sort(sorted_jobs.begin(), sorted_jobs.end(), [](const SwmJob *j1, const SwmJob *j2) -> bool {
      return j1->get_priority() != j2->get_priority() ? j1->get_priority() > j2->get_priority()
                                                      : j1->get_gang_id() < j2->get_gang_id();
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
  size_t queued = 0;
  size_t skipped = 0;
  for (size_t i = 0; i < jobs_ref->size(); i++) {
    if (events->forced_to_interrupt()) {
      return false;
    }

    const auto job = (*jobs_ref)[i];
    if (job->get_state() == "Q") {
      ++queued;
      auto &tt = (*tts)[tt_number];
      tt.set_job_id(job->get_id());

      // Check the dependencies. All dependent jobs must already be scheduled.
      uint64_t start_time_threshold = 0;
      {
        bool has_deps = false;
        for (const auto &dep : job->get_deps()) {
          auto iter = jobs_to_endtimes.find(std::get<1>(dep));
          if (iter == jobs_to_endtimes.end()) {
            swm_logi("Skip job %s: unmet dependency on job %s", job->get_id().c_str(), std::get<1>(dep).c_str());
            has_deps = true;
            break;
          }
          start_time_threshold = std::max(start_time_threshold, iter->second);
        }
        if (has_deps) {
          ++skipped;
          continue;
        }
      }

      // Check the gang id
      if (job->get_gang_id() != gang_id) {
        // Check that the new gang id is unique
        if (!job->get_gang_id().empty()) {
          auto gang_iter = known_gang_ids.find(job->get_gang_id());
          if (gang_iter != known_gang_ids.end()) {
            swm_logi("Skip job %s: gang \"%s\" is not contiguous in the queue",
                     job->get_id().c_str(),
                     job->get_gang_id().c_str());
            ++skipped;
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
      } else if (job->get_gang_id().empty()) {
        // No real gang detected, only empty identifiers. Need to reset auxiliary variables
        gang_nodes.clear();
        gang_jobs.clear();
        gang_start_time = 0;
      }

      // Schedule job and register its end time
      JobRef jr;
      error->str("");
      error->clear();
      if (!schedule_single_job(job, start_time_threshold, &gang_nodes, &tt, &jr, error)) {
        swm_logi("Can't schedule job %s: %s", job->get_id().c_str(), error->str().c_str());
        ++skipped;
        continue;
      }

      swm_logi("Scheduled job %s start=%" PRIu64 " nodes=%zu",
               job->get_id().c_str(),
               tt.get_start_time(),
               tt.get_job_nodes().size());

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
  swm_logi("FCFS finished: queued=%zu scheduled=%zu skipped=%zu", queued, tt_number, skipped);
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
  auto comp = [](const NodeRef *r1, const NodeRef *r2) -> bool { return r2->when_free() > r1->when_free(); };
  for (auto cluster_id : clusters) {
    auto iter = nodes_per_cluster_.find(cluster_id);
    if (iter == nodes_per_cluster_.end()) {
      throw std::runtime_error("FcfsImplementation::align_timetables(): internal error, no such cluster");
    }
    std::sort(iter->second.begin(), iter->second.end(), comp);
  }
}

bool FcfsImplementation::is_node_owned_by_other_job(const SwmJob &job,
                                                    const std::vector<SwmResource> &resources) const {
  const auto job_id = job.get_id();
  auto iter_res = std::find_if(resources.begin(), resources.end(), [job_id](const SwmResource &res) -> bool {
    if (res.get_name() != "job") {
      return false;
    }
    std::vector<SwmTupleAtomBuff> props = res.get_properties();
    auto iter_id = std::find_if(
        props.begin(), props.end(), [](const SwmTupleAtomBuff &prop) -> bool { return std::get<0>(prop) == "id"; });
    if (iter_id == props.end()) {
      return false;
    }

    std::string prop_id;
    if (ei_buffer_to_str(std::get<1>(*iter_id), prop_id)) {
      return false;
    }
    return res.get_name() == "job" && prop_id != job_id;
  });
  return iter_res != resources.end();
}

// Check if request does not block node to be selected for the job
bool FcfsImplementation::is_dynamic_request(const std::string &req_name) const {
  static const std::vector<std::string> dyn_req_names {
      "node", "container-image", "cloud-image", "ports", "submission-address"};
  return std::find(dyn_req_names.begin(), dyn_req_names.end(), req_name) != dyn_req_names.end();
}

bool FcfsImplementation::does_node_fit_request(const std::vector<SwmResource> &requests,
                                               const std::vector<SwmResource> &resources,
                                               std::stringstream *error) const {
  for (const auto &req : requests) {
    if (is_dynamic_request(req.get_name())) {
      continue;
    }
    const auto res_found =
        std::find_if(resources.begin(), resources.end(), [&req, &error](const SwmResource &res) -> bool {
          if (req.get_count() == 0) {
            return true;
          }
          if (req.get_name() == res.get_name() && req.get_count() <= res.get_count()) {
            auto req_props = req.get_properties();
            if (req_props.size()) {
              auto res_props = res.get_properties();
              for (SwmTupleAtomBuff &req_prop : req_props) {
                const auto prop_found =
                    std::find_if(res_props.begin(), res_props.end(), [&req_prop](auto &res_prop) -> bool {
                      if (std::get<0>(res_prop) == std::get<0>(req_prop)) {
                        std::string res_x2, req_x2;
                        ei_buffer_to_str(std::get<1>(res_prop), res_x2);
                        ei_buffer_to_str(std::get<1>(req_prop), req_x2);
                        return res_x2 == req_x2;
                      }
                      return false;
                    });
                if (prop_found == res_props.end()) {
                  return false;
                }
              }
            }
            return true;
          }
          return false;
        });
    if (res_found == resources.end()) {
      *error << "resource not found: " << req.get_name() << "; ";
      return false;
    }
  }
  return true;
}

bool FcfsImplementation::schedule_single_job(const SwmJob *job,
                                             uint64_t start_time_threshold,
                                             std::set<std::string> *busy_nodes,
                                             SwmTimetable *tt,
                                             JobRef *job_ref,
                                             std::stringstream *error) {
  // Step 0 - some preparations
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }

  // Step 1 - get requests and extract the total number of nodes
  const auto &requests = job->get_request();
  auto pred = [](const SwmResource &res) -> bool { return res.get_name() == "node"; };
  auto node_num_iter = std::find_if(requests.begin(), requests.end(), pred);
  if (node_num_iter == requests.end()) {
    *error << "job has not defined resource \"node\", unable to allocate it; ";
    return false;
  }
  auto node_num = node_num_iter->get_count();
  if (node_num < 1) {
    *error << "job's resource \"node\" must be greater or equal to 1; ";
    return false;
  }
  auto nodes_iter = nodes_per_cluster_.find(job->get_cluster_id());
  if (nodes_iter == nodes_per_cluster_.end()) {
    *error << "there is no cluster with such id (#" << job->get_cluster_id() << "); ";
    return false;
  }
  auto &nodes = nodes_iter->second;

  const auto job_nodes_vec = job->get_nodes();
  swm_logd("FCFS try job %s: request_nodes=%" PRIu64 " cluster=%s preset=%zu pool=%zu",
           job->get_id().c_str(),
           node_num,
           job->get_cluster_id().c_str(),
           job_nodes_vec.size(),
           nodes.size());

  // Step 2 - select nodes that satisfy the requirements
  std::set<std::string> node_ids_set(job_nodes_vec.begin(), job_nodes_vec.end());
  std::vector<NodeRef *> selected_nodes;
  size_t after_busy = 0;
  size_t after_preset = 0;
  size_t after_owned = 0;
  size_t after_fit = 0;

  for (auto &nr : nodes) {
    const auto node_id = nr->node()->get_id();

    // Exclude nodes from the busy list
    auto busy_iter = busy_nodes->find(node_id);
    if (busy_iter != busy_nodes->end()) {
      continue;
    }
    ++after_busy;

    // Check pre-set node ids
    if (job_nodes_vec.size()) {
      const auto found = node_ids_set.find(node_id);
      if (found == node_ids_set.end()) {
        continue;
      }
      node_ids_set.erase(found);
    }
    ++after_preset;

    const auto &resources = nr->node()->get_resources();

    if (is_node_owned_by_other_job(*job, resources)) {
      *error << "node is owned by other job; ";
      continue;
    }
    ++after_owned;

    if (does_node_fit_request(requests, resources, error)) {
      ++after_fit;
      selected_nodes.emplace_back(nr);
    }
  }

  swm_logd(
      "FCFS candidates job %s: pool=%zu after_busy=%zu after_preset=%zu "
      "after_owned=%zu after_fit=%zu",
      job->get_id().c_str(),
      nodes.size(),
      after_busy,
      after_preset,
      after_owned,
      after_fit);

  // Step 3 - cloud template nodes are elastic: one matching template can
  //          satisfy any requested node count (VMs are spawned later from it).
  //          Keep a single template id in the timetable (wm_compute expects that).
  {
    auto template_it = std::find_if(selected_nodes.begin(), selected_nodes.end(), [](const NodeRef *nr) -> bool {
      return nr->node()->get_is_template() == "true";
    });
    if (template_it != selected_nodes.end()) {
      const auto *tpl = (*template_it)->node();
      swm_logd("FCFS template collapse job %s: %s (%s); request_nodes %" PRIu64 " -> 1",
               job->get_id().c_str(),
               tpl->get_name().c_str(),
               tpl->get_id().c_str(),
               node_num);
      selected_nodes.assign(1, *template_it);
      node_num = 1;
    }
  }

  // Step 4 - the first "node_num" nodes are preferred to use
  //          because the vector are sorted by "when_free" times
  //          But we will check the following nodes as well,
  //          probably, they are placed in the better partition
  if (node_num > selected_nodes.size()) {
    *error << "not enough nodes: " << node_num << " > " << selected_nodes.size() << " (preset=" << job_nodes_vec.size()
           << " cluster_pool=" << nodes.size() << " after_fit=" << after_fit << "); ";
    return false;
  }
  auto ext_node_num = node_num;
  while (ext_node_num < selected_nodes.size() &&
         selected_nodes[ext_node_num]->when_free() == selected_nodes[node_num - 1]->when_free()) {
    ext_node_num += 1;
  }
  selected_nodes.resize(ext_node_num);

  // Step 5 - if we have a choice, select the nodes from the same partition
  if (selected_nodes.size() > node_num) {
    // We have to separate references of the selected nodes by partitions
    std::unordered_map<const SwmPartition *, std::vector<NodeRef *> > parts_to_nodes;
    for (auto &nr : selected_nodes) {
      parts_to_nodes[rh_.node_to_part(nr->node())].emplace_back(nr);
    }

    // Now we need to sort groups with nodes by their sizes to select the largest ones
    std::vector<const std::vector<NodeRef *> *> refs;
    refs.reserve(parts_to_nodes.size());
    for (const auto &rec : parts_to_nodes) {
      refs.emplace_back(&rec.second);
    }
    std::sort(refs.begin(), refs.end(), [](const std::vector<NodeRef *> *v1, const std::vector<NodeRef *> *v2) -> bool {
      return v2->size() < v1->size();
    });

    const auto *chosen_part = rh_.node_to_part((*refs.front())[0]->node());
    swm_logd("FCFS partition pick job %s: candidates=%zu need=%" PRIu64
             " parts=%zu "
             "chosen=%s (%s) size=%zu",
             job->get_id().c_str(),
             selected_nodes.size(),
             node_num,
             parts_to_nodes.size(),
             chosen_part->get_name().c_str(),
             chosen_part->get_id().c_str(),
             refs.front()->size());

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

  // Step 6 - done! We need to create and fill the timetable by node's identifiers
  //          Also we need to update/resort vector "nodes" and extend collection "busy_nodes"
  auto first_free_node =
      std::min_element(selected_nodes.begin(), selected_nodes.end(), [](const NodeRef *v1, const NodeRef *v2) -> bool {
        return v1->when_free() > v2->when_free();
      });
  uint64_t start_time = std::max(start_time_threshold, (*first_free_node)->when_free());
  tt->set_job_id(job->get_id());
  tt->set_start_time(start_time);
  std::vector<std::string> node_ids;
  node_ids.reserve(selected_nodes.size());
  std::string node_names;
  constexpr size_t kMaxLoggedNames = 8;
  for (size_t i = 0; i < selected_nodes.size(); ++i) {
    const auto pn = selected_nodes[i];
    pn->when_free() = start_time + job->get_duration();
    node_ids.push_back(pn->node()->get_id());
    busy_nodes->insert(pn->node()->get_id());
    if (i < kMaxLoggedNames) {
      if (!node_names.empty()) {
        node_names += ",";
      }
      node_names += pn->node()->get_name();
    }
  }
  if (selected_nodes.size() > kMaxLoggedNames) {
    node_names += ",+" + std::to_string(selected_nodes.size() - kMaxLoggedNames) + " more";
  }
  swm_logd("FCFS assign job %s: start=%" PRIu64 " nodes=[%s]", job->get_id().c_str(), start_time, node_names.c_str());
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
