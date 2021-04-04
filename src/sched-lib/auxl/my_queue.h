
#pragma once

#include "defs.h"

namespace swm {
namespace util {

// Thread-safe fixed-size version of queue. Blocks caller if it's overflowed.
// Will be used for command storing and etc
template <class T>
class MyQueue {
 public:
  MyQueue(size_t max_size) : queue_size_(0), queue_pos_(0) {
    if (max_size == 0) {
      throw std::runtime_error(
        "MyQueue::MyQueue(): \"max_size\" must be greater than 0");
    }
    queue_.resize(max_size);
    queue_locker_.clear();
  }

  size_t element_count() const { return queue_size_; };
  size_t size() const { return queue_.size(); }

  void push(const T &value) {
    while (true) {
      while (queue_size_ == size()) {
        std::this_thread::yield();
      }
      lock();
      if (queue_size_ < size()) {
        queue_[(queue_pos_ + queue_size_) % size()] = value;
        queue_size_ += 1;
        unlock();
        return;
      } else {
        unlock();
      }
    }
  }

  bool try_peek(T *value) {
    if (value == nullptr) {
      throw std::runtime_error("MyQueue::try_peek(): \"value\" cannot be equal to nullptr");
    }

    bool res = false;
    lock();
    if (queue_size_ > 0) {
      *value = queue_[queue_pos_];
      res = true;
    }
    unlock();
    return res;
  }

  T pop() {
    while (true) {
      while (queue_size_ == 0) {
        std::this_thread::yield();
      }
      lock();
      if (queue_size_ > 0) {
        auto value = queue_[queue_pos_];
        queue_pos_ = (queue_pos_ + 1) % size();
        queue_size_ -= 1;
        unlock();
        return value;
      }
      unlock();
    }
  }

 private:
  void lock() {
    while (queue_locker_.test_and_set())
      std::this_thread::yield();
  }
  void unlock() { queue_locker_.clear(); }

  std::vector<T> queue_;
  size_t queue_size_, queue_pos_;
  std::atomic_flag queue_locker_;
};

}
}
