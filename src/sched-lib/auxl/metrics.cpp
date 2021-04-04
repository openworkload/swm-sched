
#include "metrics.h"

namespace swm {
namespace util {

//-------------------------------
//--- Metrics::OneTypeMetrics ---
//-------------------------------

template <class T>
class Metrics::OneTypeMetrics {
 public:
  OneTypeMetrics() {
    locker_.clear();
  }
  OneTypeMetrics(const OneTypeMetrics &obj) {
    locker_.clear();
    obj.lock();
    values_ = obj.values_;
    indices_ = obj.indices_;
    obj.unlock();
  }
  void operator =(const OneTypeMetrics &) = delete;

  void register_value(uint32_t id, const std::string &name) {
    lock();
    auto iter = values_.find(id);
    if (iter != values_.end()) {
      unlock();
      throw std::runtime_error("OneTypeMetrics<T>::register_value(): value already registered");
    }
    values_.insert( { id, MetricsRecord() } );
    indices_.push_back(std::pair<uint32_t, std::string>(id, name));
    unlock();
  }

  void add_event_handler(uint32_t id, const std::function<void(T, T)> &handler) {
    auto &record = lock_and_retrieve(id);
    record.handlers.push_back(handler);
    unlock();
  }

  void export_indices(std::vector<std::pair<uint32_t, std::string> > *indices) const {
    lock();
    if (indices != nullptr) {
      indices->clear();
      indices->reserve(indices_.size());
      indices->insert(indices->begin(), indices_.cbegin(), indices_.cend());
    }
    unlock();
  }
  
  T value(uint32_t id) const {
    auto &record = lock_and_retrieve(id);
    T res = record.value;
    unlock();

    return res;
  }

  T update_value(uint32_t id, T increment) {
    auto &record = lock_and_retrieve(id);
    T old_value = record.value;
    record.value += increment;
    T new_value = record.value;
    auto handlers = record.handlers;
    unlock();

    for (auto handler : handlers) {
      handler(old_value, new_value);
    }
    return new_value;
  }

  void reset_value(uint32_t id) {
    auto &record = lock_and_retrieve(id);
    T old_value = record.value;
    record.value = T();
    T new_value = record.value;
    auto handlers = record.handlers;
    unlock();

    for (auto handler : handlers) {
      handler(old_value, new_value);
    }
  }

 private:
  struct MetricsRecord {
    T value;
    std::vector<std::function<void(T, T)> > handlers;
    MetricsRecord() : value(T()) { }
  };

  void lock() const {
    while (locker_.test_and_set()) {
      std::this_thread::yield();
    }
  }

  void unlock() const {
    locker_.clear();
  }

  const MetricsRecord &lock_and_retrieve(uint32_t id) const {
    lock();
    auto iter = values_.find(id);
    if (iter == values_.end()) {
      unlock();
      throw std::runtime_error("OneTypeMetrics<T>::lock_and_retrieve(): value not registered");
    }
    return iter->second;
  };

  MetricsRecord &lock_and_retrieve(uint32_t id) {
    lock();
    auto iter = values_.find(id);
    if (iter == values_.end()) {
      unlock();
      throw std::runtime_error("OneTypeMetrics<T>::lock_and_retrieve(): value not registered");
    }
    return iter->second;
  };

  mutable std::atomic_flag locker_;
  std::unordered_map<uint32_t, MetricsRecord > values_;
  std::vector<std::pair<uint32_t, std::string> > indices_;
};

//---------------
//--- Metrics ---
//---------------

Metrics::Metrics() {
  int_values_ = new OneTypeMetrics<int>();
  double_values_ = new OneTypeMetrics<double>();
}

Metrics::Metrics(const Metrics &obj) {
  int_values_ = new OneTypeMetrics<int>(*obj.int_values_);
  double_values_ = new OneTypeMetrics<double>(*obj.double_values_);
}

Metrics::~Metrics() {
  if (int_values_ != nullptr) {
    delete int_values_;
    int_values_ = nullptr;
  }

  if (double_values_ != nullptr) {
    delete double_values_;
    double_values_ = nullptr;
  }
}

void Metrics::register_int_value(uint32_t id, const std::string &name) {
  int_values_->register_value(id, name);
}

void Metrics::register_double_value(uint32_t id, const std::string &name) {
  double_values_->register_value(id, name);
}


void Metrics::add_int_value_handler(uint32_t id, const std::function<void(int, int)> &handler) {
  int_values_->add_event_handler(id, handler);
}

void Metrics::add_double_value_handler(uint32_t id, const std::function<void(double, double)> &handler) {
  double_values_->add_event_handler(id, handler);
}


std::vector<std::pair<uint32_t, std::string> > Metrics::int_value_indices() const {
  std::vector<std::pair<uint32_t, std::string> > res;
  int_values_->export_indices(&res);
  return res;
}

std::vector<std::pair<uint32_t, std::string > > Metrics::double_value_indices() const {
  std::vector<std::pair<uint32_t, std::string > > res;
  double_values_->export_indices(&res);
  return res;
}


int32_t Metrics::int_value(uint32_t id) const {
  return int_values_->value(id);
}

double Metrics::double_value(uint32_t id) const {
  return double_values_->value(id);
}


int32_t Metrics::update_int_value(uint32_t id, int32_t increment) {
  return int_values_->update_value(id, increment);
}

double Metrics::update_double_value(uint32_t id, double increment) {
  return double_values_->update_value(id, increment);
}


void Metrics::reset_int_value(uint32_t id) {
  int_values_->reset_value(id);
}

void Metrics::reset_double_value(uint32_t id) {
  double_values_->reset_value(id);
}

} // util
} // swm
