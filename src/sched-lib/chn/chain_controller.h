
#pragma once

#include <chrono>

#include "defs.h"
#include "chain.h"
#include "metrics_snapshot.h"

namespace swm {
namespace util {

// Allows owner to enqueue multiple async commands to chain
// Spawns personal thread to check timeouts
class ChainController {
 public:
  typedef std::function<void(bool)> exchange_callback;
  typedef std::function<void(bool,
                             const std::shared_ptr<TimetableInfoInterface> &,
                             const std::shared_ptr<MetricsSnapshot> &)> finish_callback;
  typedef std::function<void(bool,
                             const std::shared_ptr<MetricsSnapshot> &)> stats_callback;

  ChainController()
      : timeout_(0.0), finished_(false), stopped_(false),
        service_metrics_(nullptr), exchange_stage_(WAITING), exchange_traget_(nullptr) { }
  ChainController(const ChainController &) = delete;
  void operator =(const ChainController &) = delete;
  ~ChainController();
  void init(const std::shared_ptr<Chain> &chain,
            const swm::MetricsInterface *service_metrics,
            const finish_callback &clb,
            double timeout,
            std::shared_ptr<TimeCounter> timer = nullptr);
  bool finished() const;

  void invoke_exchange(const ChainController *target,
                       const exchange_callback &clb,
                       std::shared_ptr<TimeCounter> timer = nullptr);
  void invoke_interrupt(const finish_callback &clb,
                        std::shared_ptr<TimeCounter> timer = nullptr);
  void invoke_stats(const stats_callback &clb,
                    std::shared_ptr<TimeCounter> timer = nullptr);

 private:
  enum ExchangeStage {
    WAITING        = 0,       // waiting while target controller starts exchange
    TT_TAKEN       = 2,       // taked tt from target controller, not injected yet
    TT_INJECTING   = 3,       // ready for injection, expecting the same status from target ctrler
    FAILED         = 4        // something went wrong!
  };
  typedef std::chrono::high_resolution_clock clock;
  typedef std::chrono::time_point<clock> time_point;
  static double to_seconds(const time_point &end, const time_point &start) {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() * 1e-6;
  }

  void invoke(const std::function<void(bool)> &func);
  void worker_loop(const std::shared_ptr<TimeCounter> &timer);
  void lock() {
    while (locker_.test_and_set()) { std::this_thread::yield(); }
  }
  void unlock() { locker_.clear(); }

  double timeout_;
  std::thread worker_;
  std::atomic_flag locker_;

  volatile bool finished_;       // the worker thread has finished all work
  volatile bool stopped_;        // chain has completed its work, but worker thread is still active
  finish_callback finish_clb_;
  std::queue<std::function<void(bool)> > queue_;
  std::shared_ptr<Chain> chain_;
  const swm::MetricsInterface *service_metrics_;

  volatile ExchangeStage exchange_stage_;
  volatile const Chain *exchange_traget_;
};

} // util
} // swm
