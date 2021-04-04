
#pragma once

#include <iostream>

#include "defs.h"
#include "alg/algorithm_factory.h"
#include "hw/scanner.h"

namespace swm {

class Service {
 public:
  Service(const AlgorithmFactory *factory, const Scanner *scanner)
      : factory_(factory), scanner_(scanner), debug_mode_(false),
        input_(&std::cin), output_(&std::cout),
        in_queue_size_(4), out_queue_size_(4), timeout_(10.0) { }
  Service(const Service &) = delete;
  void operator =(const Service &) = delete;

  bool is_debug_mode() const { return debug_mode_; }
  void set_debug_mode(bool enabled) { debug_mode_ = enabled; }

  void set_input(std::istream *input) { input_ = input; }
  std::istream *get_input() const { return input_; }

  void set_output(std::ostream *output) { output_ = output; }
  std::ostream *get_output() const { return output_; }

  void   set_in_queue_size(size_t size) { in_queue_size_ = size; }
  size_t get_in_queue_size() const { return in_queue_size_; }

  void   set_out_queue_size(size_t size) { out_queue_size_ = size; }
  size_t get_out_queue_size() const { return out_queue_size_; }

  void   set_timeout(double seconds) { timeout_ = seconds; }
  double get_timeout() const { return timeout_; }

  void main_loop();

 private:
  const AlgorithmFactory *factory_;
  const Scanner *scanner_;
  bool debug_mode_;
  std::istream *input_;
  std::ostream *output_;
  size_t in_queue_size_;
  size_t out_queue_size_;
  double timeout_;
};

} // swm
