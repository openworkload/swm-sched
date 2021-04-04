
#pragma once

#include "defs.h"

/*
NOTE: the enums in this file must be consistent
with swm/include/wm_scheduler.hrl
*/

namespace swm {
namespace util {

enum CommandType {
  SWM_COMMAND_SCHEDULE     = 0,
  SWM_COMMAND_INTERRUPT    = 1,
  SWM_COMMAND_METRICS      = 2,
  SWM_COMMAND_EXCHANGE     = 3,
  SWM_COMMAND_CORRUPTED    = 4      // internal value, cannot be received from SWM
};
const size_t CommandTypeCount = 4;

/*enum {
  SWM_PREEMTION_DISABLED   = 0,
  SWM_PREEMTION_PRIORITY   = 1
};*/

enum DataType {
  SWM_DATA_TYPE_SCHEDULERS = 0,
  SWM_DATA_TYPE_RH         = 1,
  SWM_DATA_TYPE_JOBS       = 2,
  SWM_DATA_TYPE_GRID       = 3,
  SWM_DATA_TYPE_CLUSTERS   = 4,
  SWM_DATA_TYPE_PARTITIONS = 5,
  SWM_DATA_TYPE_NODES      = 6,
};
const size_t DataTypeCount = 7;

// TODO: autogenerate from schema.json:
const size_t SwmTimeTableTupleSize = 4;
const size_t SwmSchedulerResultTupleSize = 8;
const size_t SwmMetricTupleSize = 4;

} // util
} // swm
