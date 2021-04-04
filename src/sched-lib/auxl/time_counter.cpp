
#include "time_counter.h"

namespace swm {
namespace util {

void TimeCounter::get_times(double *astronomical, double *idling, double *working) {
  while (locker_.test_and_set()) {
    std::this_thread::yield();
  }

  auto now = my_clock::now();
  double working_add = 0.0;
  bool is_idling = true;
  for (const auto &it : thread_tps_) {
    if (it.second.first) {
      working_add += to_seconds(now, it.second.second);
      is_idling = false;
    }
  }

  if (astronomical != nullptr) {
    *astronomical = to_seconds(now, start_tp_);
  }

  if (idling != nullptr) {
    *idling = idling_time_ + (is_idling ? to_seconds(now, last_working_tp_) : 0.0);
  }

  if (working != nullptr) {
    *working = working_time_ + (is_idling ? 0.0 : working_add);
  }

  locker_.clear();
}

void TimeCounter::reset() {
  while (locker_.test_and_set()) {
    std::this_thread::yield();
  }

  for (auto &it : thread_tps_) {
    if (it.second.first) {
      locker_.clear();
      throw std::runtime_error("TimeCounter::reset(): some threads have not finished counting");
    }
  }

  start_tp_ = my_clock::now();
  last_working_tp_ = start_tp_;
  working_time_ = 0.0;
  idling_time_ = 0.0;
  locker_.clear();
}

void TimeCounter::turn_on() {
  while (locker_.test_and_set()) {
    std::this_thread::yield();
  }

  bool i_am_the_first = true;
  bool found_my_record = false;
  for (auto &it : thread_tps_) {
    i_am_the_first = i_am_the_first && !it.second.first;

    if (it.first == std::this_thread::get_id()) {
      if (it.second.first) {
        locker_.clear();
        throw std::runtime_error("TimeCounter::turn_on(): counter was not turned off");
      }

      it.second.first = true;
      it.second.second = my_clock::now();
      found_my_record = true;
    }
  }

  if (!found_my_record) {
    thread_tps_[std::this_thread::get_id()] = std::make_pair(true, my_clock::now());
  }

  if (i_am_the_first) {
    idling_time_ += to_seconds(my_clock::now(), last_working_tp_);
  }

  locker_.clear();
}

void TimeCounter::turn_off() {
  while (locker_.test_and_set()) {
    std::this_thread::yield();
  }

  bool i_am_the_last = true;
  bool found_my_record = false;
  for (auto &it : thread_tps_)
  {
    if (it.first == std::this_thread::get_id()) {
      if (!it.second.first) {
        locker_.clear();
        throw std::runtime_error("TimeCounter::turn_off(): timer was not turned on");
      }

      it.second.first = false;
      working_time_ += to_seconds(my_clock::now(), it.second.second);
      found_my_record = true;
    }

    i_am_the_last = i_am_the_last && !it.second.first;
  }

  if (!found_my_record) {
    locker_.clear();
    throw std::runtime_error("TimeCounter::turn_off(): timer was not turned on");
  }

  if (i_am_the_last) {
    last_working_tp_ = my_clock::now();
  }

  locker_.clear();
}

} // util
} // swm
