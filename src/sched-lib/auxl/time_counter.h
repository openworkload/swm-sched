
#pragma once

#include <chrono>
#include "defs.h"

namespace swm {
namespace util {

// Thread-safe counter that measures time from different threads
class TimeCounter {
 public:
  // Stack-based locker for timer
  class Lock {
   public:
    Lock(const std::shared_ptr<TimeCounter> &counter) : counter_(counter) {
      if (counter_.get() != nullptr) { counter_->turn_on(); }
    }
    Lock(const Lock &) = delete;
    void operator =(const Lock &) = delete;
    ~Lock() {
      if (counter_.get() != nullptr) { counter_->turn_off(); }
    }

   private:
     std::shared_ptr<TimeCounter> counter_;
  };

  TimeCounter()
      : start_tp_(my_clock::now()), last_working_tp_(start_tp_),
        working_time_(0.0), idling_time_(0.0) {
    locker_.clear();
  }
  TimeCounter(const TimeCounter &) = delete;
  void operator =(const TimeCounter &) = delete;

  void get_times(double *astronomical, double *idling, double *working);
  void reset();
  void turn_on();
  void turn_off();

 private:
  typedef std::chrono::high_resolution_clock my_clock;
  typedef std::chrono::time_point<my_clock> my_time_point;

  static double to_seconds(const my_time_point &end, const my_time_point &start) {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() * 1e-6;
  }

  std::atomic_flag locker_;
  std::unordered_map<std::thread::id, std::pair<bool, my_time_point> > thread_tps_;
  my_time_point start_tp_;
  my_time_point last_working_tp_;
  double working_time_;
  double idling_time_;
};

} // util
} // swm
