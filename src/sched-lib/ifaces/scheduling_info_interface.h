
#pragma once

#include <vector>

#include "wm_grid.h"
#include "wm_cluster.h"
#include "wm_partition.h"
#include "wm_node.h"
#include "wm_job.h"

extern "C" {
namespace swm {

// Simple container that is being used in SchedulingInfoInterface. Rh means Resource Hierarchy
class RhItem {
public:
 RhItem() = delete;
 RhItem(const RhItem &) = default;
 RhItem(const std::string &name, const std::string &id, std::vector<RhItem> *children_content = nullptr)
     : name_(name), id_(id) {
   if (children_content != nullptr) {
     children_ = std::move(*children_content);
   }
 }

 const std::string &name() const { return name_; }
 std::string id() const { return id_; }
 const std::vector<RhItem> &children() const { return children_; }

private:
 std::string name_;
 std::string id_;
 std::vector<RhItem> children_;
};

class SchedulingInfoInterface {
 public:
  virtual ~SchedulingInfoInterface() { }

  virtual const SwmGrid *grid() const = 0;
  virtual const std::vector<RhItem> &resource_hierarchy() const = 0;
  virtual const std::vector<const SwmCluster *> &clusters() const = 0;
  virtual const std::vector<const SwmPartition *> &parts() const = 0;
  virtual const std::vector<const SwmNode *> &nodes() const = 0;
  virtual const std::vector<const SwmJob *> &jobs() const = 0;
  virtual void print_resource_hierarchy(std::ostream *str) const = 0;
};

} // swm
} // extern "C"
