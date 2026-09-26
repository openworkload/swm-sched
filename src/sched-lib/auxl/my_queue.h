
#pragma once

#include "defs.h"

#include <condition_variable>

namespace swm {
namespace util {

// Thread-safe fixed-size queue. Blocks on push when full and on pop when empty.
template <class T>
class MyQueue {
 public:
  MyQueue(size_t max_size) : queue_size_(0), queue_pos_(0) {
    if (max_size == 0) {
      throw std::runtime_error("MyQueue::MyQueue(): \"max_size\" must be greater than 0");
    }
    queue_.resize(max_size);
  }

  size_t element_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_size_;
  }

  size_t size() const { return queue_.size(); }

  void push(const T &value) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_not_full_.wait(lock, [this] { return queue_size_ < queue_.size(); });
    queue_[(queue_pos_ + queue_size_) % queue_.size()] = value;
    queue_size_ += 1;
    lock.unlock();
    cv_not_empty_.notify_one();
  }

  bool try_peek(T *value) {
    if (value == nullptr) {
      throw std::runtime_error("MyQueue::try_peek(): \"value\" cannot be equal to nullptr");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_size_ > 0) {
      *value = queue_[queue_pos_];
      return true;
    }
    return false;
  }

  T pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_not_empty_.wait(lock, [this] { return queue_size_ > 0; });
    auto value = queue_[queue_pos_];
    queue_pos_ = (queue_pos_ + 1) % queue_.size();
    queue_size_ -= 1;
    lock.unlock();
    cv_not_full_.notify_one();
    return value;
  }

  // Blocks until an element is available or stop() is true.
  // Returns true and stores the element in *value when one was popped.
  template <typename StopPred>
  bool pop_or(StopPred stop, T *value) {
    if (value == nullptr) {
      throw std::runtime_error("MyQueue::pop_or(): \"value\" cannot be equal to nullptr");
    }

    std::unique_lock<std::mutex> lock(mutex_);
    cv_not_empty_.wait(lock, [this, &stop] { return queue_size_ > 0 || stop(); });
    if (queue_size_ == 0) {
      return false;
    }
    *value = queue_[queue_pos_];
    queue_pos_ = (queue_pos_ + 1) % queue_.size();
    queue_size_ -= 1;
    lock.unlock();
    cv_not_full_.notify_one();
    return true;
  }

  // Wake waiters (e.g. so a consumer can observe a stop flag and exit).
  void wake() {
    cv_not_empty_.notify_all();
    cv_not_full_.notify_all();
  }

 private:
  std::vector<T> queue_;
  size_t queue_size_, queue_pos_;
  mutable std::mutex mutex_;
  std::condition_variable cv_not_empty_;
  std::condition_variable cv_not_full_;
};

}  // namespace util
}  // namespace swm
