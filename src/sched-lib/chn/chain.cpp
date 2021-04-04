
#include "chain.h"

namespace swm {

Chain::~Chain() {
  // Use async operations to stop worker
  if (status_ == WORKING) {
    while (!ready_for_async_operation()) {
      std::this_thread::yield();
    }

    try { interrupt_async(); }
    catch (std::exception &ex) {
      std::cerr << "Internal error at Chain::~Chain(): failed to stop worker (\""
                << ex.what() << "\")";
      return;
    }

    while (status_ != FINISHED && status_ != INTERRUPTED) {
      std::this_thread::yield();
    }
  }

  if (worker_.joinable()) {
    worker_.join();
  }
}

void Chain::init(const std::shared_ptr<SchedulingInfoInterface> &info,
                 const std::vector<std::shared_ptr<Algorithm> > &algorithms,
                 std::shared_ptr<util::TimeCounter> timer) {
  if (info.get() == nullptr || algorithms.empty()) {
    throw std::runtime_error("Chain::init(): \"info\" and \"algorithms\" cannot be empty");
  }
  if (status_ != NOT_STARTED) {
    throw std::runtime_error("Chain::init(): object was already initialized");
  }

  status_ = WORKING;
  async_op_ = NONE;
  info_ = info;
  algorithms_ = algorithms;

  algorithms_ptrs_.resize(algorithms_.size());
  for (size_t i = 0; i < algorithms_.size(); ++i) {
    algorithms_ptrs_[i] = algorithms_[i].get();
  }

  locker_.clear();
  worker_ = std::thread([obj = this, timer]() -> void { obj->worker_loop(timer); });
}

const ChainMetrics &Chain::metrics() const {
  if (status_ == NOT_STARTED) {
    throw std::runtime_error("Chain::metrics(): object must be initialized first");
  }
  return metrics_;
}

const std::vector<const Algorithm *> &Chain::algorithms() const {
  if (status_ == NOT_STARTED) {
    throw std::runtime_error("Chain::algorithms(): object must be initialized first");
  }
  return algorithms_ptrs_;
}

std::shared_ptr<TimetableInfoInterface> Chain::intermediate_timetable() const {
  if (status_ == NOT_STARTED) {
    throw std::runtime_error("Chain::intermediate_timetable(): object must be initialized first");
  }

  // std::shared_ptr<> is not atomic in gcc. So, we need to use locker
  std::shared_ptr<TimetableInfoInterface> res;
  lock();
  res = intermediate_tt_;
  unlock();
  return res;
}

std::shared_ptr<TimetableInfoInterface> Chain::actual_timetable() const {
  if (status_ == NOT_STARTED) {
    throw std::runtime_error("Chain::actual_timetable(): object must be initialized first");
  }

  // std::shared_ptr<> is not atomic in gcc, so, need to use locker
  std::shared_ptr<TimetableInfoInterface> res;
  lock();
  res = actual_tt_;
  unlock();
  return res;
}

bool Chain::ready_for_async_operation() const {
  if (status_ == NOT_STARTED) {
    throw std::runtime_error(
      "Chain::ready_for_async_operation(): object must be initialized first");
  }

  bool res = false;
  lock();
  res = async_op_ == NONE;
  unlock();
  return res;
}

void Chain::interrupt_async() {
  if (status_ == NOT_STARTED) {
    throw std::runtime_error(
      "Chain::interrupt_async(): object must be initialized first");
  }

  lock();
  if (async_op_ != NONE) {
    unlock();
    throw std::runtime_error("Chain::interrupt_async(): chain is not ready for async operation");
  }
  if (!stopped()) {
    async_op_ = INTERRUPT;
  }
  unlock();
}

void Chain::inject_timetable_async(const std::shared_ptr<TimetableInfoInterface> &tt) {
  if (status_ == NOT_STARTED) {
    throw std::runtime_error(
      "Chain::inject_timetable_async(): object must be initialized first");
  }

  lock();
  if (async_op_ != NONE) {
    unlock();
    throw std::runtime_error(
      "Chain::inject_timetable_async(): chain is not ready for async operation");
  }

  if (!stopped()) {
    async_op_ = INJECT_TT;
    injected_tt_ = tt;
  }
  else {
    actual_tt_ = tt;
  }
  unlock();
}


bool Chain::forced_to_interrupt() const {
  bool res = false;
  lock();
  res = async_op_ == INTERRUPT || async_op_ == INJECT_TT;
  unlock();
  return res;
}

void Chain::commit_intermediate_timetable(const std::shared_ptr<TimetableInfoInterface> &tt) {
  lock();
  intermediate_tt_ = tt;
  unlock();
}

void Chain::lock() const {
  while (locker_.test_and_set()) {
    std::this_thread::yield();
  }
}

void Chain::unlock() const {
  locker_.clear();
}

void Chain::worker_loop(const std::shared_ptr<util::TimeCounter> &timer) {
  if (algorithms_.empty()) {
    return;
  }
  
  util::TimeCounter::Lock time_lock(timer);
  status_ = WORKING;
  std::stringstream errors;
  const int BUFFER_NUMBER = 2;      // buffers for ping-pong scheme, must be equal to 2
  size_t tt_cur = 0;
  std::shared_ptr<TimetableInfoInterface> tt[BUFFER_NUMBER];

  // Use the first algorithm to create timetable, others - to improve it
  bool succeeded = false;
  bool injected = false;
  for (size_t i = 0; i < algorithms_.size(); i += succeeded ? 1 : 0) {
    succeeded = (i == 0 && !injected)
            ? algorithms_[0]->create_timetable(info_.get(), this,
                                               &tt[(tt_cur + 1) % BUFFER_NUMBER], &errors)
            : algorithms_[i]->improve_timetable(tt[tt_cur].get(), this,
                                                &tt[(tt_cur + 1) % BUFFER_NUMBER], &errors);
    
    lock();
    if (!succeeded && async_op_ == NONE) { // wasn't interrupted by async op?
      std::cerr << "Failed to " << (i == 0 ? "construct" : "improve") << " timetable: "
                << errors.str() << std::endl;
      async_op_ = NONE;         // ignoring current operation
      status_ = INTERRUPTED;
      unlock();
      return;
    }

    // Store timetable as actual and clear intermidiate version
    if (succeeded) {
      actual_tt_ = tt[(tt_cur + 1) % BUFFER_NUMBER];
    }
    intermediate_tt_.reset();

    // Check for async operation
    if (async_op_ == INTERRUPT) {
      async_op_ = NONE;
      status_ = INTERRUPTED;
      unlock();
      return;
    }
    if (async_op_ == INJECT_TT) {
      async_op_ = NONE;
      status_ = WORKING;

      actual_tt_ = injected_tt_;
      tt[(tt_cur + 1) % BUFFER_NUMBER] = injected_tt_;
      injected = true;
    }
    unlock();

    // Switch buffers with actual and obsolete timetables
    tt_cur = (tt_cur + 1) % BUFFER_NUMBER;
  }

  // Done! Just need to update status
  status_ = FINISHED;
}

} // swm
