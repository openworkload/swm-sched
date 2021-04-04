
#include "scheduling_info_presets.h"
#include "scheduling_info_configurator.h"

std::shared_ptr<swm::SchedulingInfoInterface>
    SchedulingInfoPresets::one_node_one_job(const std::string &job_id) {
  SchedulingInfoConfigurator config;
  auto cluster = config.create_cluster("1", "up");
  auto part = cluster->create_partition("1", "up");
  auto node = part->create_node("1", "up", "idle");
  node->create_resources({ { "cpu", 32 },{ "mem", 68719476736 } });
  auto job = config.create_job(job_id, "1", 0);
  job->create_request("node", 1);

  swm::util::SchedulingInfo *ref = nullptr;
  std::shared_ptr<swm::SchedulingInfoInterface> obj;
  config.construct(&obj, &ref);
  return obj;
}
