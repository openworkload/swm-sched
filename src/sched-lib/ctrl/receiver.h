
#pragma once

#include <stdint.h>

#include "defs.h"
#include "commands.h"
#include "auxl/my_queue.h"


namespace swm {
namespace util {

class Receiver {
 public:  
  Receiver() : closed_(false), finished_(false), input_(nullptr), queue_(nullptr) { }
  Receiver(const Receiver &) = delete;
  void operator =(const Receiver &) = delete;
  ~Receiver();

  void init(MyQueue<std::shared_ptr<CommandInterface> > *queue, std::istream *input);
  bool finished();

 private:
  bool get_data(std::vector<std::unique_ptr<unsigned char[]> > *data,
                CommandType *cmd,
                SwmUID *uid,
                std::stringstream *errors = nullptr);
  void worker_loop();

  volatile bool closed_;              // forces the worker thread to stop
  volatile bool finished_;            // all data were wrapped into commands
  std::istream *input_;
  MyQueue<std::shared_ptr<CommandInterface> > *queue_;
  std::thread worker_;
};

} // util
} // swm
