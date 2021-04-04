
#pragma once

#include "plugin_defs.h"

// The base class for algorithm-specific contexts. Also can be used as an empty context.
class PluginContext {
 public:
  PluginContext() = delete;
  PluginContext(const PluginContext &) = delete;
  PluginContext(swm::MetricsInterface *metrics) : metrics_(metrics) { }
  void operator =(const PluginContext &) = delete;

  swm::MetricsInterface *metrics() { return metrics_; }

 private:
  swm::MetricsInterface *metrics_;
};
