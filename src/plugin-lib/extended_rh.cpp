
#include "extended_rh.h"
#include <set>

namespace swm {

// Helper that performs some checks for RH and extracts the related
// item (node, partition or cluster), significantly reduces duplicated code
template <class ITEM> static inline
const ITEM *check_rh_and_extract_item(const swm::RhItem &rh,
                                      const std::unordered_map<std::string, const ITEM *> &m,
                                      const std::string &item_name,
                                      std::set<std::string> *met_ids,
                                      std::stringstream *error) {
  auto iter = m.find(rh.id());
  if (iter == m.end()) {
    *error << item_name << " with id=" << rh.id() << " was referenced in RH but cannot be found in \""<< item_name <<"s\"";
    return nullptr;
  }
  if (rh.name() != item_name) {
    *error << "wrong structure of RH, met \"" << rh.name()
           << "\" instead of \"" << item_name <<"\"";
    return nullptr;
  }
  if (met_ids->find(rh.id()) != met_ids->end()) {
    *error << "wrong structure of RH, "
           << item_name << " #" << rh.id() << " was referenced twice";
    return nullptr;
  }
  met_ids->insert(rh.id());
  return iter->second;
}

bool ExtendedRH::parse_node(const RhItem &item,
                            std::set<std::string> *known_nodes,
                            const swm::SwmCluster *cluster,
                            const swm::SwmPartition *part,
                            std::stringstream *error) {
  auto node = check_rh_and_extract_item(item, ids_to_nodes_,
                                        "node", known_nodes, error);
  if (node == nullptr) {
    return false;
  }
  if (!item.children().empty()) {
    *error << "wrong structure of RH, node #" << item.id() << " has children";
    return false;
  }

  nodes_to_parts_[node] = part;
  nodes_to_clusters_[node] = cluster;
  return true;
}

bool ExtendedRH::parse_part(const RhItem &item,
                            std::set<std::string> *known_parts, std::set<std::string> *known_nodes,
                            const swm::SwmCluster *cluster, std::stringstream *error) {
  auto part = check_rh_and_extract_item(item, ids_to_parts_,
                                        "partition", known_parts, error);
  if (part == nullptr) {
    return false;
  }

  for (const auto &child : item.children()) {
    if (child.name() == "partition") {
      if (!parse_part(child, known_parts, known_nodes, cluster, error)) {
        return false;
      }
    }
    else if (!parse_node(child, known_nodes, cluster, part, error)) {
      return false;
    }
  }

  parts_to_clusters_[part] = cluster;
  return true;
}

bool ExtendedRH::parse_cluster(const RhItem &item, std::set<std::string> *known_clusters,
                               std::set<std::string> *known_parts, std::set<std::string> *known_nodes,
                               std::stringstream *error) {
  auto cluster = check_rh_and_extract_item(item, ids_to_clusters_,
                                           "cluster", known_clusters, error);
  if (!cluster) {
    return false;
  }

  for (const auto &part : item.children()) {
    if (!parse_part(part, known_parts, known_nodes, cluster, error)) {
      return false;
    }
  }

  return true;
}

bool ExtendedRH::init(const swm::SchedulingInfoInterface *sched_info, std::stringstream *error) {
  if (sched_info == nullptr) {
    throw std::runtime_error("ExtendedRH::init(): \"sched_info\" cannot be equal to nullptr");
  }
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }

  // Construct maps "id -> cluster", "id -> partition" and "id -> node"
  // Assume that all identifiers are unique
  for (const auto &cluster : sched_info->clusters()) {
    ids_to_clusters_[cluster->get_id()] = cluster;
  }
  for (const auto &part : sched_info->parts()) {
    ids_to_parts_[part->get_id()] = part;
  }
  for (const auto &node : sched_info->nodes()) {
    ids_to_nodes_[node->get_id()] = node;
  }

  // Parse RH and construct maps "node -> cluster", "node -> partition" and "partition -> cluster"
  // Start from the level of grid or clusters (depends on SWM scenario)
  const std::vector<swm::RhItem> *root_rh = &sched_info->resource_hierarchy();
  if (root_rh->size() == 1 && (*root_rh)[0].name() == "grid") {
    root_rh = &(*root_rh)[0].children();
  }

  std::set<std::string> cluster_ids;
  std::set<std::string> part_ids;
  std::set<std::string> node_ids;
  for (const auto &cluster : *root_rh) {
    if (!parse_cluster(cluster, &cluster_ids, &part_ids, &node_ids, error)) {
      return false;
    }
  }

  return true;
}

} // swm
