
#include "service_metrics.h"

namespace swm {
namespace util {

ServiceMetrics::ServiceMetrics() {
  metrics_.register_int_value(REQUESTS_ID, "the total number of processed requests");
}

} // util
} // swm
