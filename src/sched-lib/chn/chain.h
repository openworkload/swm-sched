
#pragma once

#include "defs.h"
#include "auxl/time_counter.h"
#include "chain_metrics.h"
#include "alg/algorithm.h"
#include "ifaces/metrics_interface.h"
#include "ifaces/timetable_info_interface.h"
#include "ifaces/plugin_events_interface.h"

namespace swm {

// Groups set of algorithms into chain. Spawns personal worker thread, provides thread-safe
// async methods for basic management from single master thread. More complex functionality
// implemented via ChainController.
class Chain : private PluginEventsInterface {
 public:
  enum StatusType {
    NOT_STARTED   = 0,      // init() must be called
    WORKING       = 1,      // constructing tt, doing something useful
    INTERRUPTED   = 2,      // tt construction process interrupted by someone or due to errors
    FINISHED      = 3       // tt constructed without errors and can be used
  };
  
  Chain() : status_(NOT_STARTED), async_op_(NONE) { }
  void operator =(const Chain &) = delete;
  ~Chain();

  void init(const std::shared_ptr<SchedulingInfoInterface> &info,
            const std::vector<std::shared_ptr<Algorithm> > &algorithms,
            std::shared_ptr<util::TimeCounter> timer = nullptr);
  const ChainMetrics &metrics() const;
  const std::vector<const Algorithm *> &algorithms() const;
  std::shared_ptr<TimetableInfoInterface> intermediate_timetable() const;
  std::shared_ptr<TimetableInfoInterface> actual_timetable() const;

  StatusType status() const { return status_; }
  bool stopped() const { return status_ != WORKING; }
  bool ready_for_async_operation() const;
  void interrupt_async();
  void inject_timetable_async(const std::shared_ptr<TimetableInfoInterface> &tt);

 protected:
  virtual bool forced_to_interrupt() const  override;
  virtual void commit_intermediate_timetable(
    const std::shared_ptr<TimetableInfoInterface> &tt) override;
 
 private:
  enum AsyncOperationType {
    NONE      = 0,        // nothing to do
    INTERRUPT = 1,        // chain must be interrupted asap
    INJECT_TT = 2         // new tt "injected_tt_" must be injected asap
  };

  void lock() const;
  void unlock() const;
  void worker_loop(const std::shared_ptr<util::TimeCounter> &timer);

  mutable std::atomic_flag locker_;
  std::thread worker_;

  volatile StatusType status_;
  volatile AsyncOperationType async_op_;
  ChainMetrics metrics_;
  std::shared_ptr<SchedulingInfoInterface> info_;
  std::vector<std::shared_ptr<Algorithm> > algorithms_;
  std::vector<const Algorithm *> algorithms_ptrs_;
  
  std::shared_ptr<TimetableInfoInterface> injected_tt_;
  std::shared_ptr<TimetableInfoInterface> intermediate_tt_;
  std::shared_ptr<TimetableInfoInterface> actual_tt_;
};

} // swm
