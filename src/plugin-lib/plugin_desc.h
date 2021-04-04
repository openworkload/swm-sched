
#pragma once

#include "plugin_defs.h"

class PluginDesc : public swm::AlgorithmDescInterface {
 public:
  PluginDesc() = delete;
  PluginDesc(const std::string &family, const std::string &version,
             swm::ComputeUnitInterface::Type cu)
      : family_(family), version_(version), cu_(cu) { }
  PluginDesc(const PluginDesc &) = delete;
  void operator =(const PluginDesc &) = delete;

  virtual const std::string &family_id() const override { return family_; }
  virtual const std::string &version() const override { return version_; }
  virtual swm::ComputeUnitInterface::Type device_type() const override { return cu_; }

 private:
  std::string family_;
  std::string version_;
  swm::ComputeUnitInterface::Type cu_;
};
