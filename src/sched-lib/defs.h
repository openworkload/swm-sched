
#pragma once

#include "wm_cluster.h"
#include "wm_grid.h"
#include "wm_io.h"
#include "wm_job.h"
#include "wm_node.h"
#include "wm_partition.h"
#include "wm_timetable.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

typedef std::string SwmUID;
