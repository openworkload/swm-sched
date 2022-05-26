
#include "service.h"

#include <algorithm>

#include "receiver.h"
#include "processor.h"
#include "sender.h"
#include "chn/metrics_snapshot.h"

using namespace swm;

void Service::main_loop() {
  util::MyQueue<std::shared_ptr<util::CommandInterface>> in_queue(in_queue_size_);
  util::MyQueue<std::shared_ptr<util::ResponseInterface>> out_queue(out_queue_size_);

  // Start processing asynchronously
  util::Receiver receiver;
  receiver.init(&in_queue, input_);

  util::Processor processor;
  processor.init(factory_, scanner_, &in_queue, &out_queue, timeout_);

  util::Sender sender;
  sender.init(&out_queue, output_);

  // Wait until all incoming requests are not received
  while (!receiver.finished()) {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // FIXME should it be less?
  }

  // Current thread will be blocked by close() until all requests are not performed
  processor.close();
  sender.close();
}
