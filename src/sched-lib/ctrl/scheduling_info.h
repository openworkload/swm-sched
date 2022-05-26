
#pragma once

#include "defs.h"

#include "ifaces/scheduling_info_interface.h"

namespace swm {
namespace util {

class SchedulingInfo : public SchedulingInfoInterface {
 public:
  SchedulingInfo() : are_references_valid_(true) { };

  virtual const SwmGrid *grid() const override { return &grid_; }
  void set_grid(const SwmGrid &grid) { grid_ = grid; }

  virtual const std::vector<RhItem> &resource_hierarchy() const override { return rh_; }
  std::vector<RhItem> &resource_hierarchy_vector() { return rh_; }

  virtual const std::vector<const SwmCluster *> &clusters() const override;
  std::vector<SwmCluster> &clusters_vector() { are_references_valid_ = false; return clusters_; }

  virtual const std::vector<const SwmPartition *> &parts() const override;
  std::vector<SwmPartition> &parts_vector() { are_references_valid_ = false; return parts_; }
  
  virtual const std::vector<const SwmNode *> &nodes() const override;
  std::vector<SwmNode> &nodes_vector() { are_references_valid_ = false; return nodes_; }

  virtual const std::vector<const SwmJob *> &jobs() const override;
  std::vector<SwmJob> &jobs_vector() { are_references_valid_ = false; return jobs_; }
  
  void validate_references();
  virtual void print_resource_hierarchy(std::ostream *str) const override;

private:
  bool are_references_valid_;
  SwmGrid grid_;
  std::vector<RhItem> rh_;
  std::vector<SwmCluster> clusters_;
  std::vector<const SwmCluster *> cluster_ptrs_;
  std::vector<SwmPartition> parts_;
  std::vector<const SwmPartition *> part_ptrs_;
  std::vector<SwmNode> nodes_;
  std::vector<const SwmNode *> node_ptrs_;
  std::vector<SwmJob> jobs_;
  std::vector<const SwmJob *> job_ptrs_;
};

} // util
} // swm
