
#pragma once

#include "defs.h"
#include "algorithm.h"

namespace swm {

class AlgorithmFactory {
 public:
  AlgorithmFactory() { }
  ~AlgorithmFactory() { }

  bool load_plugins(const std::string &path, std::stringstream *error = nullptr);
  bool create(const AlgorithmDescInterface *desc,
              std::shared_ptr<Algorithm> *res,
              std::stringstream *error = nullptr) const;

  const std::vector<const AlgorithmDescInterface *> &known_algorithms() const { return alg_ptrs_; }

 private:
  std::vector<std::shared_ptr<util::LibBinding> > lib_bindings_;  // all loaded plugins
  std::vector<const AlgorithmDescInterface *> alg_ptrs_;    // references to actual and working
                                                            // plugins only. But, currently, all
                                                            // loaded plugins are marked as working
};

} // swm
