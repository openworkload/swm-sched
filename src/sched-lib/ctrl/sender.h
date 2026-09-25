
#pragma once

#include "auxl/my_queue.h"
#include "defs.h"
#include "responses.h"

namespace swm {
namespace util {

class Sender {
 public:
  Sender() : closed_(false), output_(nullptr), queue_(nullptr) {}
  Sender(const Sender &) = delete;
  ~Sender();
  void operator=(const Sender &) = delete;

  void init(MyQueue<std::shared_ptr<ResponseInterface> > *queue, std::ostream *output);
  void close();

 private:
  void worker_thread();

  volatile bool closed_;  // forces the worker thread to stop
  std::ostream *output_;
  MyQueue<std::shared_ptr<ResponseInterface> > *queue_;
  std::thread worker_;
};

}  // namespace util
}  // namespace swm
