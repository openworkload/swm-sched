
#pragma once

#include "test_defs.h"
#include "ctrl/scheduling_info.h"

// Allows the developer to configure virtual cloud and jobs in an elegant way
// The constructed cloud can have wrong structure
// It's just because we want to test error handling as well
class SchedulingInfoConfigurator {
 public:
  class PartitionConfigurator;
  class ClusterConfigurator;

  class NodeConfigurator {
   public:
    NodeConfigurator() = delete;
    NodeConfigurator(const NodeConfigurator &) = default;
    void operator =(const NodeConfigurator &) = delete;

    void create_resources(const std::vector<std::pair<std::string, uint64_t> > &vals);
    void create_resource(const std::string &name, uint64_t value) {
      create_resources({ { name, value } });
    }

   private:
    NodeConfigurator(const std::string &id,
                     const std::string &state_power,
                     const std::string &state_alloc);
    const swm::SwmNode *build(swm::util::SchedulingInfo *obj);

    std::vector<std::function<void(swm::SwmNode *)> > setters_;
   friend class SchedulingInfoConfigurator::PartitionConfigurator;
  };

  class PartitionConfigurator {
   public:
    PartitionConfigurator() = delete;
    PartitionConfigurator(const PartitionConfigurator &) = default;
    void operator =(const PartitionConfigurator &) = delete;

    NodeConfigurator *create_node(const std::string &id, const std::string &state_power,
                                  const std::string &state_alloc);

   private:
    PartitionConfigurator(const std::string &id, const std::string &state);
    const swm::SwmPartition *build(swm::util::SchedulingInfo *obj,
                                   std::vector<swm::RhItem> *child_rh);

    std::vector<std::function<void(swm::SwmPartition *)> > setters_;
    std::vector<std::shared_ptr<NodeConfigurator> > nodes_;
   friend class SchedulingInfoConfigurator::ClusterConfigurator;
  };

  class ClusterConfigurator {
   public:
    ClusterConfigurator() = delete;
    ClusterConfigurator(const ClusterConfigurator &) = default;
    void operator =(const ClusterConfigurator &) = delete;

    PartitionConfigurator *create_partition(const std::string &id, const std::string &state);

   private:
    ClusterConfigurator(const std::string &id, const std::string &state);
    swm::SwmCluster *build(swm::util::SchedulingInfo *obj,
                           std::vector<swm::RhItem> *child_rh);

    std::vector<std::function<void(swm::SwmCluster *)> > setters_;
    std::vector<std::shared_ptr<PartitionConfigurator> > parts_;
   friend class SchedulingInfoConfigurator;
  };

  class JobConfigurator {
   public:
    JobConfigurator() = delete;
    JobConfigurator(const JobConfigurator &) = default;
    void operator =(const JobConfigurator &) = delete;

    void create_requests(const std::vector<std::pair<std::string, uint64_t> > &vals);
    void create_request(const std::string &name, uint64_t value) {
      create_requests( { { name, value } } );
    }
    void set_state(const std::string &state);
    void set_dependencies(const std::vector<std::string> &job_ids);
    void set_dependency(const std::string &job_id) { set_dependencies({ job_id }); }
    void set_priority(uint64_t priority);
    void set_gang_id(const std::string &gang_id);
    void set_node_ids(const std::vector<std::string> &node_ids);

   private:
    JobConfigurator(const std::string &job_id, const std::string &cluster_id, uint64_t duration);
    const swm::SwmJob *build(swm::util::SchedulingInfo *obj);

    std::vector<std::function<void(swm::SwmJob *)> > setters_;
   friend class SchedulingInfoConfigurator;
  };

  SchedulingInfoConfigurator() = default;
  SchedulingInfoConfigurator(const SchedulingInfoConfigurator &) = delete;
  void operator =(const SchedulingInfoConfigurator &) = delete;

  JobConfigurator *create_job(const std::string &job, const std::string &cluster_id, uint64_t duration);
  ClusterConfigurator *create_cluster(const std::string &id, const std::string &state);
  void construct(std::shared_ptr<swm::SchedulingInfoInterface> *obj,
                                 swm::util::SchedulingInfo **casted_ref = nullptr);

 private:
  std::vector<std::shared_ptr<JobConfigurator> > jobs_;
  std::vector<std::shared_ptr<ClusterConfigurator> > clusters_;
};
