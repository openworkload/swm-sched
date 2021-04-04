
#include "chain_controller.h"

namespace swm {
namespace util {

ChainController::~ChainController() {
  stopped_ = true;
  if (worker_.joinable()) {
    worker_.join();
  }
}

void ChainController::init(const std::shared_ptr<Chain> &chain,
                           const swm::MetricsInterface *service_metrics,
                           const finish_callback &clb,
                           double timeout,
                           std::shared_ptr<TimeCounter> timer) {
  if (chain.get() == nullptr || service_metrics == nullptr) {
    throw std::runtime_error(
      "ChainController::init(): \"chain\" and \"service_metrics\" cannot be equal to nullptr");
  }

  if (timeout <= 0.0) {
    throw std::runtime_error("ChainController::init(): \"timeout\" must be greater than zero");
  }

  if (chain_.get() != nullptr || service_metrics_ != nullptr) {
    throw std::runtime_error("ChainController::init(): object already initialized");
  }

  chain_ = chain;
  service_metrics_ = service_metrics;
  finish_clb_ = clb;
  timeout_ = timeout;
  finished_ = false;
  stopped_ = false;
  exchange_stage_ = WAITING;
  exchange_traget_ = nullptr;

  locker_.clear();
  worker_ = std::thread([obj = this, timer]() -> void { obj->worker_loop(timer); });
}

void ChainController::invoke(const std::function<void(bool)> &func) {
  if (chain_.get() == nullptr || service_metrics_ == nullptr) {
    throw std::runtime_error("ChainController::invoke(): object must be initialized first");
  }

  // Trying to delegate request to worker thread
  bool placed = false;
  lock();
  if ((placed = !stopped_)) {
    queue_.push(func);
  }
  unlock();

  // The worker thread was already stopped, just skipping request
  if (!placed) {
    func(true);
  }
}

bool ChainController::finished() const {
  if (chain_.get() == nullptr || service_metrics_ == nullptr) {
    throw std::runtime_error("ChainController::finished(): object must be initialized first");
  }

  return finished_;
}

void ChainController::invoke_exchange(const ChainController *target,
                                      const exchange_callback &clb,
                                      std::shared_ptr<TimeCounter> timer) {
  auto func = [clb,
               chain = chain_.get(),
               exch_stage = &exchange_stage_,
               exch_chain_ptr = &exchange_traget_,
               target_chain = target->chain_.get(),
               target_exch_stage = &target->exchange_stage_,
               target_exch_chain_ptr = &target->exchange_traget_,
               timeout = timeout_,
               timer](bool skipped) -> void {
    TimeCounter::Lock time_lock(timer);
    if (!skipped && !chain->stopped() && !target_chain->stopped()) {
      auto t_start = clock::now();

      // Waiting while target controller is ready for exchange
      // Important: exchange with our chain, not some other else
      *exch_stage = WAITING;
      *exch_chain_ptr = target_chain;
      while (*target_exch_chain_ptr != chain &&
             to_seconds(clock::now(), t_start) < timeout) {
        std::this_thread::yield();
      }
      if (*target_exch_chain_ptr != chain) {
        *exch_stage = FAILED;
        *exch_chain_ptr = nullptr;
        clb(false);
        return;
      }

      // Taking target's tt before it has injected our
      auto tt = target_chain->actual_timetable();
      if (tt.get() == nullptr) {
        *exch_stage = FAILED;
        *exch_chain_ptr = nullptr;
        clb(false);
        return;
      }
      *exch_stage = TT_TAKEN;
      while (*target_exch_stage != TT_TAKEN &&
             *target_exch_stage != TT_INJECTING &&
             *target_exch_stage != FAILED &&
             to_seconds(clock::now(), t_start) < timeout) {
        std::this_thread::yield();
      }
      if (*target_exch_stage != TT_TAKEN &&
          *target_exch_stage != TT_INJECTING) {
        *exch_stage = FAILED;
        *exch_chain_ptr = nullptr;
        clb(false);
        return;
      }
      
      // Waiting while our chain will is ready for injection
      while (!chain->stopped() &&
             !chain->ready_for_async_operation() &&
             to_seconds(clock::now(), t_start) < timeout) {
        std::this_thread::yield();
      }
      if (!chain->ready_for_async_operation() || chain->stopped()) {
        *exch_stage = FAILED;
        *exch_chain_ptr = nullptr;
        clb(false);
        return;
      }

      // Ok, expecting the same status on target
      *exch_stage = TT_INJECTING;
      while (*target_exch_stage != TT_INJECTING &&
             *target_exch_stage != FAILED &&
             to_seconds(clock::now(), t_start) < timeout) {
        std::this_thread::yield();
      }
      if (*target_exch_stage != TT_INJECTING) {
        *exch_stage = FAILED;
        *exch_chain_ptr = nullptr;
        clb(false);
        return;
      }

      // Injecting tt taken from the target chain
      try {
        chain->inject_timetable_async(target_chain->actual_timetable());
      }
      catch (std::exception &ex) {
        *exch_stage = FAILED;
        std::cerr << "Exception from ChainController::invoke_exchange(): "
                  << ex.what() << std::endl;
      }

      // Done! Notifying the caller
      *exch_chain_ptr = nullptr;
      clb(true);
    }
    else {
      clb(false);
    }
  };

  invoke(func);
}

void ChainController::invoke_interrupt(const finish_callback &clb,
                                       std::shared_ptr<TimeCounter> timer) {
  auto func = [clb,
               srv_metrics = service_metrics_,
               chain = chain_.get(),
               hard_interruption_flag = &stopped_,
               timeout = timeout_,
               timer](bool skipped) -> void {
    TimeCounter::Lock time_lock(timer);
    std::shared_ptr<TimetableInfoInterface> tt;
    std::shared_ptr<MetricsSnapshot> metrics;
    
    if (!skipped && chain->status() == Chain::WORKING) {
      auto t_start = clock::now();

      // First of all, checking that chain is ready for operation
      while (!chain->stopped() &&
             !chain->ready_for_async_operation() &&
             (to_seconds(clock::now(), t_start) < timeout)) {
        std::this_thread::yield();
      }

      // Not ready? Performing "hard" interruption
      if (!chain->ready_for_async_operation()) {
        *hard_interruption_flag = true;
        metrics.reset(new MetricsSnapshot(*srv_metrics, *chain));
        clb(true, tt, metrics);
        return;
      }

      // Trying to enqueue async request
      try {
        chain->interrupt_async();
      }
      catch (std::exception &ex) {
        std::cerr << "Exception from ChainController::invoke_interrupt(): "
                  << ex.what() << std::endl;
        clb(false, tt, metrics);
        return;
      }

      // Ok, waiting for "soft" interruption
      while (!chain->stopped() &&
             (to_seconds(clock::now(), t_start) < timeout)) {
        std::this_thread::yield();
      }

      // Checking that it's actually stopped. If not - starting "hard" interruption
      metrics.reset(new MetricsSnapshot(*srv_metrics,  *chain));
      if (chain->stopped()) {
        clb(chain->status() == Chain::INTERRUPTED, tt, metrics);
      }
      else {
        *hard_interruption_flag = true;
        clb(true, tt, metrics);
      }
    }
    else {
      clb(false, tt, metrics);
    }
  };

  invoke(func);
}

void ChainController::invoke_stats(const stats_callback &clb,
                                   std::shared_ptr<TimeCounter> timer) {
  auto func = [clb,
               serv_metrics = service_metrics_,
               chain = chain_.get(),
               timer](bool skipped) -> void {
    TimeCounter::Lock lock(timer);
    if (!skipped && !chain->stopped()) {
      std::shared_ptr<MetricsSnapshot> snapshot(new MetricsSnapshot(*serv_metrics, *chain));
      clb(true, snapshot);
    }
    else {
      clb(false, std::shared_ptr<MetricsSnapshot>());
    }
  };

  invoke(func);
}

void ChainController::worker_loop(const std::shared_ptr<TimeCounter> &timer) {
  // Processing all incoming requests, time will be measured by callbacks
  stopped_ = false;
  finished_ = false;
  while (!stopped_ && !chain_->stopped()) { // beware, "stopped_" can be set by invoked function!
    if (!queue_.empty()) {
      lock();
      auto func = queue_.front();
      queue_.pop();
      unlock();
      
      try { func(false); }
      catch (std::exception &ex) {
        std::cerr << "Exception from ChainController::worker_loop(): " << ex.what() << std::endl;
        stopped_ = true;
      }
    }
    std::this_thread::yield();
  }

  // Skipping the last requests
  TimeCounter::Lock time_lock(timer);
  lock();
  bool succeeded = !stopped_ && chain_->status() == Chain::FINISHED;// no errors and interruptions?
  while (!queue_.empty()) {
    auto &func = queue_.front();
    
    try { func(true); }
    catch (std::exception &ex) {
      std::cerr << "Exception from ChainController::worker_loop(): " << ex.what() << std::endl;
    }

    queue_.pop();
  }
  stopped_ = true;
  unlock();

  // Notifying caller about result
  finish_clb_(succeeded, chain_->actual_timetable(),
              std::shared_ptr<MetricsSnapshot>(new MetricsSnapshot(*service_metrics_, *chain_)));
  finished_ = true;
}

} // util
} // swm
