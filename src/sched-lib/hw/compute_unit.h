
#pragma once

#include "defs.h"

#include "ifaces/compute_unit_interface.h"

namespace swm {

class Scanner; // to be defined as a friend

class ComputeUnit : public ComputeUnitInterface {
 public:
  virtual Type device_type() const override { return type_; }
  virtual int device_number() const override { return num_; }
  virtual const std::string &name() const override { return name_; }
  virtual size_t cores() const override { return cores_; }
  virtual double frequency_mhz() const override { return freq_; }

 private:
  ComputeUnit(const ComputeUnit &) = delete;
  ComputeUnit(Type type, int num, const std::string &name, size_t cores, double freq)
      : type_(type), num_(num), name_(name), cores_(cores), freq_(freq) {}
  void operator =(const ComputeUnit &) = delete;

  Type type_;
  int num_;
  std::string name_;
  size_t cores_;
  double freq_;
  friend class Scanner;
};

} // swm
