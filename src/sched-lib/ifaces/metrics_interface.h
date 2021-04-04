
#pragma once

#include "defs.h"

extern "C" {
namespace swm {

// Interface for collection with metrics
// Note: std::vector<> returned by value, not by reference, due to thread safety
class MetricsInterface {
 public:
  virtual ~MetricsInterface() { }

  virtual void register_int_value(uint32_t id, const std::string &name) = 0;
  virtual std::vector<std::pair<uint32_t, std::string> > int_value_indices() const = 0;
  virtual int32_t int_value(uint32_t id) const = 0;
  virtual int32_t update_int_value(uint32_t id, int32_t increment) = 0;
  virtual void reset_int_value(uint32_t id) = 0;

  virtual void register_double_value(uint32_t id, const std::string &name) = 0;
  virtual std::vector<std::pair<uint32_t, std::string> > double_value_indices() const = 0;
  virtual double double_value(uint32_t id) const = 0;
  virtual double update_double_value(uint32_t id, double increment) = 0;
  virtual void reset_double_value(uint32_t id) = 0;

  virtual std::shared_ptr<MetricsInterface> clone() const = 0;
};

} // swm
} // extern "C"
