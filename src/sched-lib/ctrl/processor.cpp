
#include "processor.h"

#include <algorithm>
#include <thread> // for sleep

#include "hw/scanner.h"
#include "alg/algorithm_factory.h"

namespace swm {
namespace util {

Processor::Processor()
    : timeout_(0.0), closed_(false),
      factory_(nullptr), scanner_(nullptr),
      in_queue_(nullptr), out_queue_(nullptr) {
}

Processor::~Processor() {
  closed_ = true;
  if (worker_.joinable()) {
    worker_.join();
  }

  // Chain's threads were stopped by worker, no need to clear collections
}

void Processor::init(const AlgorithmFactory *factory, 
                     const Scanner *scanner,
                     MyQueue<std::shared_ptr<CommandInterface> > *in_queue,
                     MyQueue<std::shared_ptr<ResponseInterface> > *out_queue,
                     double timeout) {
  // Fatal errors
  if (in_queue_ != nullptr || out_queue_ != nullptr) {
    throw std::runtime_error("Processor::init(): object was already initialized");
  }

  if (in_queue == nullptr || out_queue == nullptr ||
      scanner == nullptr || factory == nullptr) {
    throw std::runtime_error(
      "Processor::init(): factories, in- and out- queues cannot be defined as nullptr");
  }

  if (timeout <= 0.0) {
    throw std::runtime_error("Processor::init(): \"timeout\" must be defined as positive number");
  }

  // Not a fatal error
  // Such situation can be actual for tests, or can occur due to wrong configuration
  if (timeout < 0.1) {
    std::cerr << "Potential bug in Processor::init(): "
              << "timeout value is too small, requests will be refused" << std::endl;
  }

  metrics_.reset(new ServiceMetrics());
  factory_ = factory;
  scanner_ = scanner;
  in_queue_ = in_queue;
  out_queue_ = out_queue;
  timeout_ = timeout;
  closed_ = false;

  worker_ = std::thread([obj = this]() -> void { obj->worker_thread(); });
}

void Processor::close() {
  if (in_queue_ == nullptr || out_queue_ == nullptr) {
    throw std::runtime_error("Processor::close(): object must be initialized first");
  }

  closed_ = true;
  if (worker_.joinable()) {
    worker_.join();
  }

  factory_ = nullptr;
  scanner_ = nullptr;
  in_queue_ = nullptr;
  out_queue_ = nullptr;
}

bool Processor::create_algorithms(const AlgorithmFactory *factory,
                                  const Scanner *scanner,
                                  const std::vector<ScheduleCommand::AlgorithmSpec> &specs,
                                  std::vector<std::shared_ptr<Algorithm> > *res,
                                  std::stringstream *errors) {
  std::stringstream errors_;
  if (errors == nullptr) {
    errors = &errors_;
  }

  if (factory == nullptr || res == nullptr) {
    throw std::runtime_error(
      "Processor::create_algorithms(): \"factory\" and \"res\" cannot be equal to nullptr");
  }
  if (specs.empty()) {
    *errors << "cannot create algorithm without its specification";
    return false;
  }

  // First of all, we have to find suitable descriptor for each specification
  const auto &descriptors = factory->known_algorithms();
  std::vector<const AlgorithmDescInterface *> selected;
  selected.resize(specs.size());
  for (size_t i = 0; i < specs.size(); i++) {
    // Predicate for "wise" selection
    auto pred = [spec = &specs[i]](const AlgorithmDescInterface *desc) -> bool {
      // The same family?
      if (desc->family_id() != spec->family()) {
        return false;
      }

      // Does version match?
      std::string ver;
      if (spec->version_specified(&ver) && ver != desc->version()) {
        return false;
      }

      // Does compute unit match?
      ComputeUnitInterface::Type cu;
      if (spec->compute_unit_specified(&cu) && cu != desc->device_type()) {
        return false;
      }

      // Found!
      return true;
    };

    // Check that such algorithm exists
    auto it = std::find_if(descriptors.begin(), descriptors.end(), pred);
    if (it == descriptors.end()) {
      *errors << "failed to find implementation for algorithm \"" << specs[i].family() << "\"";
      
      std::string ver;
      if (specs[i].version_specified(&ver)) {
        *errors << ", version=\"" << ver << "\"";
      }

      ComputeUnitInterface::Type cu;
      if (specs[i].compute_unit_specified(&cu)) {
        *errors << ", device=#" << (int)cu;
      }
      return false;
    }

    // Store descriptor for the following creation
    selected[i] = *it;
  }

  // Now, we can create algorithm instances and bind them to CPU
  res->clear();
  res->resize(selected.size());
  for (size_t i = 0; i < selected.size(); i++) {
    std::shared_ptr<Algorithm> alg;
    if (!factory->create(selected[i], &alg, errors) ||
        !alg->bind_to(scanner->cpu(), errors)) {
      res->clear();
      return false;
    }
    (*res)[i] = alg;
  }
  return true;
}

void Processor::respond_chain_not_found(MyQueue<std::shared_ptr<ResponseInterface> > *queue,
                                        const std::shared_ptr<CommandContext> &context,
                                        const SwmUID &chain_id) {
  std::cerr << "Processor::worker_thread(): failed to perform request "
            << "(UID=\"" << context->id() << "\") because "
            << "target chain (UID=\"" << chain_id << "\") was not found."
            << std::endl;
  queue->push(std::shared_ptr<ResponseInterface>(new util::EmptyResponse(context, false)));
}

void Processor::respond_chain_already_exists(MyQueue<std::shared_ptr<ResponseInterface> > *queue,
                                             const std::shared_ptr<CommandContext> &context,
                                             const SwmUID &chain_id) {
  std::cerr << "Processor::worker_thread(): failed to perform request "
            << "(UID=\"" << context->id() << "\") because "
            << "chain with UID=\"" << chain_id << "\" already exists."
            << std::endl;
  queue->push(std::shared_ptr<ResponseInterface>(new util::EmptyResponse(context, false)));
}

void Processor::worker_thread() {
  // Stop when: owner called close(), no new requests, all zombies are killed
  while (!closed_ || in_queue_->element_count() != 0 || !chains_.empty()) {
    if (in_queue_->element_count() != 0) {
      auto req = in_queue_->pop();
      TimeCounter::Lock time_lock(req->context()->timer());

      try {
        switch (req->type()) {

          // Create new chain, start the asynchronous construction of timetable
          case SWM_COMMAND_SCHEDULE: {
            auto sreq = static_cast<ScheduleCommand *>(req.get());
            if (chains_.find(sreq->context()->id()) != chains_.end()) {
              respond_chain_already_exists(out_queue_, sreq->context(), sreq->context()->id());
              break;
            }

            std::stringstream errors;
            std::vector<std::shared_ptr<Algorithm> > algs;
            if (!create_algorithms(factory_, scanner_, sreq->schedulers(), &algs, &errors)) {
              std::cerr << "Processor::worker_thread(): failed to create algorithms for "
                        << "request with ID=\"" << sreq->context()->id() << "\", details: "
                        << errors.str() << std::endl;
              out_queue_->push(std::shared_ptr<ResponseInterface>(
                new EmptyResponse(sreq->context(), false)));
              break;
            }
            std::shared_ptr<Chain> chain;
            chain.reset(new Chain());
            chain->init(sreq->scheduling_info(), algs, sreq->context()->timer());

            std::shared_ptr<ChainController> controller;
            auto clb = [queue = out_queue_,
                        ctx = sreq->context()]
                       (bool succeeded,
                        const std::shared_ptr<TimetableInfoInterface> &tt,
                        const std::shared_ptr<MetricsSnapshot> &m) -> void {
              std::shared_ptr<ResponseInterface> resp;
              if (succeeded) {
                resp.reset(new util::TimetableResponse(ctx, tt, m));
              }
              else {
                resp.reset(new util::EmptyResponse(ctx, false));
              }
              queue->push(resp);
            };
            controller.reset(new ChainController());
            controller->init(chain, &metrics_->object(), clb, timeout_, sreq->context()->timer());
            chains_.insert(std::make_pair(sreq->context()->id(), controller));

            break;
          }
          // Stop the chain execution
          case SWM_COMMAND_INTERRUPT: {
            auto ireq = static_cast<InterruptCommand *>(req.get());

            auto it = chains_.find(ireq->chain());
            if (it == chains_.end()) {
              respond_chain_not_found(out_queue_, ireq->context(), ireq->chain());
              break;
            }

            it->second->invoke_interrupt([queue = out_queue_, ctx = ireq->context()]
                                         (bool succeeded,
                                          const std::shared_ptr<TimetableInfoInterface> &,
                                          const std::shared_ptr<MetricsSnapshot> &) -> void {
              queue->push(std::shared_ptr<ResponseInterface>(
                new util::EmptyResponse(ctx, succeeded)));
            }, ireq->context()->timer());

            break;
          }

          // Take snapshot of the targets metrics and warp it into response
          case SWM_COMMAND_METRICS: {
            auto mreq = static_cast<MetricsCommand *>(req.get());

            auto it = chains_.find(mreq->chain());
            if (it == chains_.end()) {
              respond_chain_not_found(out_queue_, mreq->context(), mreq->chain());
              break;
            }

            it->second->invoke_stats([queue = out_queue_,
                                      ctx = mreq->context()]
                                     (bool succeeded,
                                      const std::shared_ptr<MetricsSnapshot> &m) -> void {
              std::shared_ptr<ResponseInterface> resp;
              if (succeeded) {
                resp.reset(new util::MetricsResponse(ctx, m));
              }
              else {
                resp.reset(new util::EmptyResponse(ctx, false));
              }
              queue->push(resp);
            }, mreq->context()->timer());

            break;
          }

          // Exchange timetables from two chains
          case SWM_COMMAND_EXCHANGE: {
            auto ereq = static_cast<ExchangeCommand *>(req.get());

            auto src = chains_.find(ereq->source_chain());
            if (src == chains_.end()) {
              respond_chain_not_found(out_queue_, ereq->context(), ereq->source_chain());
              break;
            }
            auto trg = chains_.find(ereq->target_chain());
            if (trg == chains_.end()) {
              respond_chain_not_found(out_queue_, ereq->context(), ereq->target_chain());
              break;
            }

            src->second->invoke_exchange(trg->second.get(),
                                         [q = out_queue_,
                                          ctx = ereq->context()](bool succeeded) -> void {
              q->push(std::shared_ptr<ResponseInterface>(new EmptyResponse(ctx, succeeded)));
            }, ereq->context()->timer());
            trg->second->invoke_exchange(src->second.get(), [](bool) -> void {}, ereq->context()->timer());

            break;
          }

          // Command was not parsed, just notify about it
          case SWM_COMMAND_CORRUPTED: {
            out_queue_->push(std::shared_ptr<ResponseInterface>(
              new util::EmptyResponse(req->context(), false)));
            break;
          }
        } // switch
      } // try
      catch (std::exception &ex) {
        std::cerr << "Exception from Processor::worker_thread():"
                  << "failed to process request with UID=\"" << req->context()->id()
                  << "\", details: " << ex.what() << std::endl;
      }

      metrics_->update_requests(1);
    } // if

    // To kill zombies: remove and release chains with stopped threads
    // Assuming that the total number of chains is small enough
    auto it = chains_.begin();
    while (it != chains_.end()) {
      auto chain = it->second.get();
      if (chain->finished()) {
        try { it = chains_.erase(it); }
        catch (std::exception &ex) {
          std::cerr << "Exception from Processor::worker_thread(): failed to release chain, " << ex.what() << std::endl;
        }
      }
      else {
        ++it;
      }
    } // while

    std::this_thread::sleep_for(std::chrono::milliseconds(2));  // TODO: wait for some event / conditional variable?
    std::this_thread::yield();
  } // while
}

} // util
} // swm
