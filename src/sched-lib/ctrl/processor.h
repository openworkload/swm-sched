
#pragma once

#include "defs.h"

#include "commands.h"
#include "responses.h"
#include "service_metrics.h"
#include "auxl/my_queue.h"
#include "chn/chain_controller.h"

namespace swm {
namespace util {

// Processor for incoming requests. Not a CPU or something like that
class Processor {
 public:
  Processor();
  Processor(const Processor &) = delete;
  ~Processor();
  void operator =(const Processor &) = delete;

  void init(const AlgorithmFactory *factory,
            const Scanner *scanner,
            MyQueue<std::shared_ptr<CommandInterface> > *in_queue,
            MyQueue<std::shared_ptr<ResponseInterface> > *out_queue,
            double timeout);
  void close();

 private:
  static bool create_algorithms(const AlgorithmFactory *factory,
                                const Scanner *scanner,
                                const std::vector<ScheduleCommand::AlgorithmSpec> &specs,
                                std::vector<std::shared_ptr<Algorithm> > *res,
                                std::stringstream *errors = nullptr);
  static void respond_chain_not_found(MyQueue<std::shared_ptr<ResponseInterface> > *queue,
                                      const std::shared_ptr<CommandContext> &context,
                                      const SwmUID &chain_id);
  static void respond_chain_already_exists(MyQueue<std::shared_ptr<ResponseInterface> > *queue,
                                           const std::shared_ptr<CommandContext> &context,
                                           const SwmUID &chain_id);
  void worker_thread();
  
  double timeout_;
  std::thread worker_;
  volatile bool closed_;                            // forces to stop waiting for new requests

  std::shared_ptr<ServiceMetrics> metrics_;         // as pointer because we need to reset them
  const AlgorithmFactory *factory_;
  const Scanner *scanner_;
  MyQueue<std::shared_ptr<CommandInterface> > *in_queue_;
  MyQueue<std::shared_ptr<ResponseInterface> > *out_queue_;
  std::unordered_map<SwmUID, std::shared_ptr<ChainController> > chains_;
};

} // util
} // swm
