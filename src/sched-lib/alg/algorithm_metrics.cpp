
#include "algorithm_metrics.h"

namespace swm {

AlgorithmMetrics::AlgorithmMetrics() {
  metrics_.register_int_value(SCHEDULED_JOBS_ID, "the total number of scheduled jobs");

  // TODO: add events in such way
  metrics_.add_int_value_handler(SCHEDULED_JOBS_ID, [](int32_t old_value, int32_t new_value) -> void {
    if (old_value == 0 && new_value >= 1) {
      // std::cerr << "We-e-e-e! The first job scheduled!" << std::endl;
    }
  });
}

}  // namespace swm
