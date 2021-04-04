
#pragma once

#include "plugin_defs.h"
#include <set>

namespace swm {

// Auxiliary class that maps nodes to partitions and clasters
// Also checks correcteness of the original RH because it can be built
// by unit tests or even SWM in a wrong way
class ExtendedRH {
 public:
  ExtendedRH() = default;
  ExtendedRH(const ExtendedRH &) = delete;
  void operator =(const ExtendedRH &) = delete;
  bool init(const swm::SchedulingInfoInterface *sched_info, std::stringstream *error = nullptr);

  const swm::SwmCluster *id_to_cluster(std::string cluster_id) const {
    return id_to_something(ids_to_clusters_, "cluster", cluster_id);
  }
  const swm::SwmPartition *id_to_part(std::string part_id) const {
    return id_to_something(ids_to_parts_, "partition", part_id);
  }
  const swm::SwmNode *id_to_node(std::string node_id) const {
    return id_to_something(ids_to_nodes_, "node", node_id);
  }
  
  const swm::SwmCluster *part_to_cluster(const swm::SwmPartition *part) const {
    return something_to_something(parts_to_clusters_, part);
  }
  const swm::SwmCluster *node_to_cluster(const swm::SwmNode *node) const {
    return something_to_something(nodes_to_clusters_, node);
  }
  const swm::SwmPartition *node_to_part(const swm::SwmNode *node) const {
    return something_to_something(nodes_to_parts_, node);
  }

 private:
  template <class T>
  const T *id_to_something(const std::unordered_map<std::string, const T *> &m,
                           const std::string &obj_name, std::string id) const {
    auto iter = m.find(id);
    if (iter == m.end()) {
      std::stringstream message;
      message << "ExtendedRH::id_to_something(): cannot find "
              << obj_name << " with ID #" << id;
      throw std::runtime_error(message.str());
    }
    return iter->second;
  }

  template <class T1, class T2>
  const T1 *something_to_something(const std::unordered_map<const T2 *, const T1 *> &m,
                                   const T2 *val) const {
    auto iter = m.find(val);
    if (iter == m.end()) {
      throw std::runtime_error(
        "ExtendedRH::something_to_something(): failed to find such element");
    }
    return iter->second;
  }

  // The following methods parse RH item and store values to instance's collections
  bool parse_node(const RhItem &item, std::set<std::string> *known_nodes,
                  const swm::SwmCluster *cluster, const swm::SwmPartition *part,
                  std::stringstream *error);
  bool parse_part(const RhItem &item,
                  std::set<std::string> *known_parts, std::set<std::string> *known_nodes,
                  const swm::SwmCluster *cluster, std::stringstream *error);
  bool parse_cluster(const RhItem &item, std::set<std::string> *known_clusters,
                     std::set<std::string> *known_parts, std::set<std::string> *known_nodes,
                     std::stringstream *error);

  std::unordered_map<std::string, const swm::SwmCluster *> ids_to_clusters_;
  std::unordered_map<std::string, const swm::SwmPartition *> ids_to_parts_;
  std::unordered_map<std::string, const swm::SwmNode *> ids_to_nodes_;

  std::unordered_map<const swm::SwmPartition *, const swm::SwmCluster *> parts_to_clusters_;
  std::unordered_map<const swm::SwmNode *, const swm::SwmCluster *> nodes_to_clusters_;
  std::unordered_map<const swm::SwmNode *, const swm::SwmPartition *> nodes_to_parts_;
};

} // swm
