
#include "scheduling_info.h"

namespace swm {
namespace util {

const std::vector<const SwmCluster *> &SchedulingInfo::clusters() const {
  if (!are_references_valid_) {
    throw std::runtime_error("SchedulingInfo::clusters(): references must be validated first");
  }
  return cluster_ptrs_;
}

const std::vector<const SwmPartition *> &SchedulingInfo::parts() const {
  if (!are_references_valid_) {
    throw std::runtime_error("SchedulingInfo::parts(): references must be validated first");
  }
  return part_ptrs_;
}

const std::vector<const SwmNode *> &SchedulingInfo::nodes() const {
  if (!are_references_valid_) {
    throw std::runtime_error("SchedulingInfo::nodes(): references must be validated first");
  }
  return node_ptrs_;
}

const std::vector<const SwmJob *> &SchedulingInfo::jobs() const {
  if (!are_references_valid_) {
    throw std::runtime_error("SchedulingInfo::jobs(): references must be validated first");
  }
  return job_ptrs_;
}

template <class VAL>
static inline void validate_references_templated(const std::vector<VAL> *vals, std::vector<const VAL *> *ptrs) {
  ptrs->clear();
  if (vals->size() != 0) {
    ptrs->resize(vals->size());
    for (size_t i = 0; i < vals->size(); ++i)
      (*ptrs)[i] = &(*vals)[i];
  }
}

void SchedulingInfo::validate_references() {
  if (!are_references_valid_) {
    validate_references_templated<SwmCluster>(&clusters_, &cluster_ptrs_);
    validate_references_templated<SwmPartition>(&parts_, &part_ptrs_);
    validate_references_templated<SwmNode>(&nodes_, &node_ptrs_);
    validate_references_templated<SwmJob>(&jobs_, &job_ptrs_);
    are_references_valid_ = true;
  }
}

static inline void print_resource_hierarchy_helper(const RhItem &item,
                                                   std::ostream *str,
                                                   size_t shift) {
  *str << std::string(shift * 2, ' ');
  *str << "{" << item.name() << ", " << item.id() << "}";
  *str << std::endl;
  for (const auto &sub_item : item.children()) {
    print_resource_hierarchy_helper(sub_item, str, shift + 1);
  }
}

void SchedulingInfo::print_resource_hierarchy(std::ostream *str) {
  if (str != nullptr) {
    for (size_t i = 0; i < rh_.size(); ++i) {
      print_resource_hierarchy_helper(rh_[i], str, 0);
    }
  }
}

} // util
} // swm
