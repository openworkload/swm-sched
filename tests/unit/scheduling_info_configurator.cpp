
#include "scheduling_info_configurator.h"

//----------------------------------------------------
//--- SchedulingInfoConfigurator::NodeConfigurator ---
//----------------------------------------------------

SchedulingInfoConfigurator::NodeConfigurator::NodeConfigurator(const std::string &id,
                                                               const std::string &state_power,
                                                               const std::string &state_alloc) {
  setters_.emplace_back([id, state_power, state_alloc](swm::SwmNode *node) -> void {
    std::stringstream ss;
    ss << "node #" << id;
    node->set_name(ss.str());

    node->set_id(id);
    node->set_state_power(state_power);
    node->set_state_alloc(state_alloc);
  });
}

void SchedulingInfoConfigurator::NodeConfigurator::create_resources(const std::vector<
                                                                     std::pair<std::string,
                                                                               uint64_t> > &vals) {
  std::vector<swm::SwmResource> resources;
  for (auto &val : vals) {
    swm::SwmResource r;
    r.set_name(val.first);
    r.set_count(val.second);
    r.set_usage_time(0);
    resources.push_back(r);
  }
  setters_.emplace_back([resources](swm::SwmNode *node) -> void {
    auto tmp = node->get_resources();
    tmp.insert(tmp.end(), resources.begin(), resources.end());
    node->set_resources(tmp);
  });
}

const swm::SwmNode *
    SchedulingInfoConfigurator::NodeConfigurator::build(swm::util::SchedulingInfo *obj) {
  swm::SwmNode node;
  for (const auto &setter : setters_) {
    setter(&node);
  }
  auto &nodes = obj->nodes_vector();
  nodes.emplace_back(node);
  return &nodes[nodes.size() - 1];
}

//---------------------------------------------------------
//--- SchedulingInfoConfigurator::PartitionConfigurator ---
//---------------------------------------------------------

SchedulingInfoConfigurator::PartitionConfigurator::PartitionConfigurator(const std::string &id,
                                                                         const std::string &state) {
  setters_.emplace_back([id, state](swm::SwmPartition *part) -> void {
    std::stringstream ss;
    ss << "partition #" << id;
    part->set_name(ss.str());

    part->set_id(id);
    part->set_state(state);
  });
}

SchedulingInfoConfigurator::NodeConfigurator *
SchedulingInfoConfigurator::PartitionConfigurator::create_node(const std::string &id,
                                                               const std::string &state_power,
                                                               const std::string &state_alloc) {
  nodes_.emplace_back(new SchedulingInfoConfigurator::NodeConfigurator(id, state_power, state_alloc));
  return nodes_[nodes_.size() - 1].get();
}

const swm::SwmPartition *
    SchedulingInfoConfigurator::PartitionConfigurator::build(swm::util::SchedulingInfo *obj,
  std::vector<swm::RhItem> *child_rh) {
  swm::SwmPartition part;
  for (const auto setter : setters_) {
    setter(&part);
  }

  child_rh->reserve(nodes_.size());
  std::vector<std::string> node_ids;
  node_ids.reserve(nodes_.size());

  for (auto &node_config : nodes_) {
    const auto node = node_config->build(obj);
    child_rh->emplace_back(swm::RhItem("node", node->get_id()));
    node_ids.emplace_back(node->get_id());
  }
  part.set_nodes(node_ids);

  auto &parts = obj->parts_vector();
  parts.emplace_back(part);
  return &parts[parts.size() - 1];
}

//-------------------------------------------------------
//--- SchedulingInfoConfigurator::ClusterConfigurator ---
//-------------------------------------------------------

SchedulingInfoConfigurator::ClusterConfigurator::ClusterConfigurator(const std::string &id,
                                                                     const std::string &state) {
  setters_.emplace_back([id, state](swm::SwmCluster *cluster) -> void {
    std::stringstream ss;
    ss << "cluster #" << id;
    cluster->set_name(ss.str());
    cluster->set_id(id);
    cluster->set_state(state);
  });
}

SchedulingInfoConfigurator::PartitionConfigurator *
    SchedulingInfoConfigurator::ClusterConfigurator::create_partition(const std::string &id,
  const std::string &state) {
  parts_.emplace_back(new PartitionConfigurator(id, state));
  return parts_[parts_.size() - 1].get();
}

swm::SwmCluster *
    SchedulingInfoConfigurator::ClusterConfigurator::build(swm::util::SchedulingInfo *obj,
  std::vector<swm::RhItem> *child_rh) {
  swm::SwmCluster res;
  for (const auto &setter : setters_) {
    setter(&res);
  }

  child_rh->reserve(parts_.size());
  std::vector<std::string> part_ids;
  part_ids.reserve(parts_.size());

  for (auto &part_config : parts_) {
    std::vector<swm::RhItem> child_children;
    const auto *part = part_config->build(obj, &child_children);
    child_rh->emplace_back(swm::RhItem("partition", part->get_id(), &child_children));
    part_ids.emplace_back(part->get_id());
  }
  res.set_partitions(part_ids);

  auto &clusters = obj->clusters_vector();
  clusters.emplace_back(res);
  return &clusters[clusters.size() - 1];
}

//---------------------------------------------------
//--- SchedulingInfoConfigurator::JobConfigurator ---
//---------------------------------------------------

SchedulingInfoConfigurator::JobConfigurator::JobConfigurator(const std::string &job_id,
                                                             const std::string &cluster_id,
                                                             uint64_t duration) {
  setters_.emplace_back([job_id, cluster_id, duration](swm::SwmJob *job) -> void {
    std::stringstream ss;
    ss << "job #\"" << job_id << "\"";
    job->set_name(ss.str());

    job->set_id(job_id);
    job->set_state("Q");
    job->set_cluster_id(cluster_id);
    job->set_duration(duration);
    job->set_priority(0);
  });
}

void SchedulingInfoConfigurator::JobConfigurator::create_requests(const std::vector<
                                                                    std::pair<std::string,
                                                                              uint64_t> > &vals) {
  std::vector<swm::SwmResource> resources;
  for (auto &val : vals) {
    swm::SwmResource r;
    r.set_name(val.first);
    r.set_count(val.second);
    r.set_usage_time(0);
    resources.push_back(r);
  }
  setters_.emplace_back([resources](swm::SwmJob *job) -> void {
    auto tmp = job->get_request();
    tmp.insert(tmp.end(), resources.begin(), resources.end());
    job->set_request(tmp);
  });
}

void SchedulingInfoConfigurator::JobConfigurator::set_state(const std::string &state) {
  setters_.emplace_back([state](swm::SwmJob *job) -> void { job->set_state(state); });
}

void SchedulingInfoConfigurator::JobConfigurator
    ::set_dependencies(const std::vector<std::string> &job_ids) {
  setters_.emplace_back([job_ids](swm::SwmJob *job) -> void {
    std::vector<swm::SwmTupleAtomStr> deps(job_ids.size());
    for (size_t i = 0; i < job_ids.size(); ++i) {
      std::get<1>(deps[i]) = job_ids[i];
    }
    job->set_deps(deps);
  });
}

void SchedulingInfoConfigurator::JobConfigurator::set_priority(uint64_t priority) {
  setters_.emplace_back([priority](swm::SwmJob *job) -> void {
    job->set_priority(priority);
  });
}

void SchedulingInfoConfigurator::JobConfigurator::set_gang_id(const std::string &gang_id) {
  setters_.emplace_back([gang_id](swm::SwmJob *job) -> void {
    job->set_gang_id(gang_id);
  });
}

void SchedulingInfoConfigurator::JobConfigurator::set_node_ids(const std::vector<std::string> &node_ids) {
  setters_.emplace_back([node_ids](swm::SwmJob *job) -> void {
    job->set_nodes(std::move(node_ids));
  });
}

const swm::SwmJob *
    SchedulingInfoConfigurator::JobConfigurator::build(swm::util::SchedulingInfo *obj) {
  swm::SwmJob job;
  for (const auto &setter : setters_) {
    setter(&job);
  }

  auto &jobs = obj->jobs_vector();
  jobs.emplace_back(job);
  return &jobs[jobs.size() - 1];
}

//-----------------------------------
//--- SchedulingInfoConfigurator ----
//-----------------------------------

SchedulingInfoConfigurator::JobConfigurator *
    SchedulingInfoConfigurator::create_job(const std::string &job_id,
                                           const std::string &cluster_id,
                                           uint64_t duration) {
  jobs_.emplace_back(new JobConfigurator(job_id, cluster_id, duration));
  return jobs_[jobs_.size() - 1].get();
}

SchedulingInfoConfigurator::ClusterConfigurator *
    SchedulingInfoConfigurator::create_cluster(const std::string &id, const std::string &state) {
  clusters_.emplace_back(new ClusterConfigurator(id, state));
  return clusters_[clusters_.size() - 1].get();
}

void SchedulingInfoConfigurator::construct(std::shared_ptr<swm::SchedulingInfoInterface> *obj,
                                           swm::util::SchedulingInfo **casted_ref) {
  if (obj == nullptr) {
    throw std::runtime_error("GridConfigurator::export(): \"obj\" cannot be equal to nullptr");
  }
  swm::util::SchedulingInfo *ref;
  obj->reset(ref = new swm::util::SchedulingInfo());
  if (casted_ref != nullptr) {
    *casted_ref = ref;
  }

  swm::SwmGrid grid;
  grid.set_name("grid");
  grid.set_state("up");

  std::vector<swm::RhItem> &rh = ref->resource_hierarchy_vector();
  rh.reserve(clusters_.size());
  std::vector<std::string> cluster_ids;
  cluster_ids.reserve(clusters_.size());
  for (auto &cluster_config : clusters_) {
    std::vector<swm::RhItem> child_children;
    const auto cluster = cluster_config->build(ref, &child_children);
    rh.emplace_back(swm::RhItem("cluster", cluster->get_id(), &child_children));
    cluster_ids.emplace_back(cluster->get_id());
  }

  grid.set_clusters(cluster_ids);
  ref->set_grid(grid);

  std::vector<swm::SwmJob> &jobs = ref->jobs_vector();
  jobs.reserve(jobs_.size());
  for (auto &job_config : jobs_) {
    job_config->build(ref);
  }

  ref->validate_references();
}
