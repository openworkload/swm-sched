
#pragma once

#include "defs.h"
#include "compute_unit.h"

namespace swm {

class Scanner {
 public:
  Scanner() : inited_(false) {}
  bool scan();
    
  const ComputeUnit *cpu() const {
    return cpu_.get();
  }
    
  const std::vector<const ComputeUnit *> &gpus() const {
    return gpus_ptrs_;
  }

 private:
  static std::string find_cpu_name();
  static size_t find_cpu_cores();
  static double find_cpu_freq();

  bool inited_;
  std::unique_ptr<ComputeUnit> cpu_;
  std::vector<std::unique_ptr<ComputeUnit> > gpus_;
  std::vector<const ComputeUnit *> gpus_ptrs_;
};

} // swm
