
#pragma once

#include "defs.h"
#include "ifaces/metrics_interface.h"

namespace swm {
namespace util {

// Thread-safe implementation of the MetricsInterface interface
// Also adds event functionality
class Metrics : public MetricsInterface {
 public:
  Metrics();
  Metrics(const Metrics &);
  void operator =(const Metrics &) = delete;
  virtual ~Metrics();

  void add_int_value_handler(uint32_t id, const std::function<void(int, int)> &handler);
  virtual void register_int_value(uint32_t id, const std::string &name) override;
  virtual std::vector<std::pair<uint32_t, std::string> > int_value_indices() const override;
  virtual int32_t int_value(uint32_t id) const override;
  virtual int32_t update_int_value(uint32_t id, int32_t increment) override;
  virtual void reset_int_value(uint32_t id) override;

  void add_double_value_handler(uint32_t id, const std::function<void(double, double)> &handler);
  virtual void register_double_value(uint32_t id, const std::string &name) override;
  virtual std::vector<std::pair<uint32_t, std::string > > double_value_indices() const override;
  virtual double double_value(uint32_t id) const override;
  virtual double update_double_value(uint32_t id, double increment) override;
  virtual void reset_double_value(uint32_t id) override;

  virtual std::shared_ptr<MetricsInterface> clone() const override {
    return std::shared_ptr<MetricsInterface>(new Metrics(*this));
  }

 private:
  // Templated implementation of all logic, located in .cpp
  template <class T> class OneTypeMetrics;

  OneTypeMetrics<int> *int_values_;
  OneTypeMetrics<double> *double_values_;
};

} // util
} // swm

