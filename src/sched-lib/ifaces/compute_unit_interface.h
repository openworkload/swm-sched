
#pragma once

#include <string>

extern "C" {
namespace swm {

class ComputeUnitInterface {
 public:
  enum Type {
    Cpu = 0,
    Gpu = 1
  };

  virtual ~ComputeUnitInterface() { }

  virtual Type device_type() const = 0;
  virtual int device_number() const = 0;
  virtual const std::string &name() const = 0;
  virtual size_t cores() const = 0;
  virtual double frequency_mhz() const = 0;
};

} // swm
} // extern "C"
