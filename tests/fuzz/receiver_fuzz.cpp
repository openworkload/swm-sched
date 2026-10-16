// Optional libFuzzer harness for Receiver::get_data + ScheduleCommand::init.
// Build with: cmake -DSWM_ENABLE_FUZZ=ON ...
// Run: ./bin/swm-sched-receiver-fuzz [corpus_dir]

#include "ctrl/command_context.h"
#include "ctrl/commands.h"
#include "ctrl/constants.h"
#include "ctrl/receiver.h"
#include "ctrl/receiver_slots.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (data == nullptr || size == 0) {
    return 0;
  }
  // Cap input so the harness itself stays light; wire length cap is separate.
  if (size > swm::util::kMaxReceiverSliceBytes + 64) {
    size = swm::util::kMaxReceiverSliceBytes + 64;
  }

  std::string bytes(reinterpret_cast<const char *>(data), size);
  std::istringstream istr(bytes);
  swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface>> queue(2);
  swm::util::Receiver receiver;
  try {
    receiver.init(&queue, &istr);
  } catch (...) {
    return 0;
  }
  while (!receiver.finished() && queue.element_count() < queue.size()) {
    std::this_thread::yield();
  }
  while (queue.element_count() > 0) {
    (void)queue.pop();
  }
  return 0;
}
