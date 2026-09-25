
#pragma once

#include "compute_unit_interface.h"

#include <string>

extern "C" {
namespace swm {

class AlgorithmDescInterface {
 public:
  virtual ~AlgorithmDescInterface() {}

  virtual const std::string &family_id() const = 0;
  virtual const std::string &version() const = 0;
  virtual ComputeUnitInterface::Type device_type() const = 0;
};

}  // namespace swm
}  // extern "C"
