
#pragma once

#include <functional>

#include "defs.h"
#include "constants.h"
#include "scheduling_info.h"
#include "command_context.h"
#include "auxl/time_counter.h"
#include "ifaces/compute_unit_interface.h"


namespace swm {
namespace util {

class Receiver;

class CommandInterface {
 public:
  CommandInterface(const CommandInterface &) = delete;
  virtual ~CommandInterface() { }
  void operator =(const CommandInterface &) = delete;

  virtual const std::shared_ptr<CommandContext> &context() const = 0;
  virtual CommandType type() const = 0;

 protected:
  CommandInterface() {}
  virtual bool init(const std::vector<std::unique_ptr<unsigned char[]> > &data,
                    std::stringstream *errors) = 0;
 friend class Receiver;
};

class CorruptedCommand : public CommandInterface {
 public:
  CorruptedCommand(const std::shared_ptr<CommandContext> &context) : context_(context) { }
  virtual const std::shared_ptr<CommandContext> &context() const override { return context_; }
  virtual CommandType type() const { return SWM_COMMAND_CORRUPTED; }

 protected:
   virtual bool init(const std::vector<std::unique_ptr<unsigned char[]> > &,
                     std::stringstream *) {
     // Important: for compatibility purposes, always returns true
     return true;
   }

 private:
  std::shared_ptr<CommandContext> context_;
};

class ScheduleCommand : public CommandInterface {
 public:
  // Which type of the algorithm must be selected?
  class AlgorithmSpec {
  public:
    AlgorithmSpec(const std::string &family_id,
                  const std::string *version = nullptr,
                  const ComputeUnitInterface::Type *cu = nullptr);

    const std::string &family() const { return family_; }
    bool version_specified(std::string *version = nullptr) const;
    bool compute_unit_specified(ComputeUnitInterface::Type *cu_type = nullptr) const;

  private:
    std::string family_;
    bool has_version_; std::string version_;
    bool has_cu_; ComputeUnitInterface::Type cu_;
  };

  // Version for unit tests only!
  ScheduleCommand(const std::shared_ptr<CommandContext> &context,
                  const std::vector<AlgorithmSpec> &schedulers,
                  const std::shared_ptr<SchedulingInfoInterface> &sched_info)
      : context_(context), schedulers_(schedulers), sched_info_ptr_(sched_info) {
    sched_info_ = static_cast<SchedulingInfo *>(sched_info_ptr_.get());
  }
  ScheduleCommand(const std::shared_ptr<CommandContext> &context) : context_(context) { }
  const std::vector<AlgorithmSpec> &schedulers() const { return schedulers_; }
  const std::shared_ptr<SchedulingInfoInterface> &scheduling_info() const { return sched_info_ptr_; }
  virtual const std::shared_ptr<CommandContext> &context() const override { return context_; }
  virtual CommandType type() const override { return SWM_COMMAND_SCHEDULE; };

 protected:
  bool init(const std::vector<std::unique_ptr<unsigned char[]> > &data,
            std::stringstream *errors = nullptr) override;

 private:
  bool apply_schedulers(ETERM *term, std::stringstream *error = nullptr);
  bool apply_jobs(ETERM* terms, std::stringstream *error = nullptr);
  bool apply_rh(ETERM* terms, std::stringstream *error = nullptr);
  bool apply_grid(ETERM* terms, std::stringstream *error = nullptr);
  bool apply_clusters(ETERM* terms, std::stringstream *error = nullptr);
  bool apply_partitions(ETERM* terms, std::stringstream *error = nullptr);
  bool apply_nodes(ETERM* terms, std::stringstream *error = nullptr);
  
  std::shared_ptr<CommandContext> context_;
  std::vector<AlgorithmSpec> schedulers_;
  std::shared_ptr<SchedulingInfoInterface> sched_info_ptr_;
  SchedulingInfo *sched_info_;
};


class InterruptCommand : public CommandInterface {
 public:
  // For unit tests only!
  InterruptCommand(const std::shared_ptr<CommandContext> &context, const SwmUID &chain)
      : context_(context), chain_(chain) { }
  InterruptCommand(const std::shared_ptr<CommandContext> &context)
      : context_(context), chain_(SwmUID()) { }
  const SwmUID &chain() const { return chain_; }
  virtual const std::shared_ptr<CommandContext> &context() const override { return context_; }
  virtual CommandType type() const override { return SWM_COMMAND_INTERRUPT; };

 protected:
  bool init(const std::vector<std::unique_ptr<unsigned char[]> > &data,
            std::stringstream *errors = nullptr) override;

 private:
  std::shared_ptr<CommandContext> context_;
  SwmUID chain_;
};


class MetricsCommand : public CommandInterface {
 public:
  // For unit tests only!
  MetricsCommand(const std::shared_ptr<CommandContext> &context, const SwmUID &chain)
    : context_(context), chain_(chain) { }
  MetricsCommand(const std::shared_ptr<CommandContext> &context)
      : context_(context), chain_(SwmUID()) { }
  const SwmUID &chain() const { return chain_; }
  virtual const std::shared_ptr<CommandContext> &context() const override { return context_; }
  virtual CommandType type() const override { return SWM_COMMAND_METRICS; }

 protected:
  bool init(const std::vector<std::unique_ptr<unsigned char[]> > &,
            std::stringstream *) override;

 private:
  std::shared_ptr<CommandContext> context_;
  SwmUID chain_;
};


class ExchangeCommand : public CommandInterface {
 public:
  // For unit tests only!
  ExchangeCommand(const std::shared_ptr<CommandContext> &context,
                  const SwmUID &source_chain, const SwmUID &target_chain)
      : context_(context), source_chain_(source_chain), target_chain_(target_chain) { }
  ExchangeCommand(const std::shared_ptr<CommandContext> &context)
      : context_(context), source_chain_(SwmUID()), target_chain_(SwmUID()) { }
  const SwmUID &source_chain() const { return source_chain_; }
  const SwmUID &target_chain() const { return target_chain_; }
  virtual const std::shared_ptr<CommandContext> &context() const override { return context_; }
  virtual CommandType type() const override { return SWM_COMMAND_EXCHANGE; };

 protected:
  bool init(const std::vector<std::unique_ptr<unsigned char[]> > &data,
            std::stringstream *errors) override;

 private:
  std::shared_ptr<CommandContext> context_;
  SwmUID source_chain_;
  SwmUID target_chain_;
};

} // util
} // swm
