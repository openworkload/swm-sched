
#include "sender.h"

namespace swm {
namespace util {

Sender::~Sender() {
  closed_ = true;
  if (queue_ != nullptr) {
    queue_->wake();
  }
  if (worker_.joinable()) {
    worker_.join();
  }
}

void Sender::init(MyQueue<std::shared_ptr<ResponseInterface> > *queue, std::ostream *output) {
  if (queue_ != nullptr || output_ != nullptr) {
    throw std::runtime_error("Sender::init(): object was already initialized");
  }
  if (queue == nullptr || output == nullptr) {
    throw std::runtime_error("Sender::init(): \"queue\" and \"output\" cannot be equal to nullptr");
  }

  queue_ = queue;
  output_ = output;
  closed_ = false;
  worker_ = std::thread([me = this]() -> void { me->worker_thread(); });
}

void Sender::close() {
  if (queue_ == nullptr || output_ == nullptr) {
    throw std::runtime_error("Sender::close(): object must be initialized first");
  }

  closed_ = true;
  queue_->wake();
  if (worker_.joinable()) {
    worker_.join();
  }

  queue_ = nullptr;
  output_ = nullptr;
}

void Sender::worker_thread() {
  while (!closed_ || queue_->element_count() > 0) {
    std::shared_ptr<ResponseInterface> resp;
    if (!queue_->pop_or([this] { return closed_.load(); }, &resp)) {
      // Stop requested and queue is empty.
      continue;
    }

    try {
      if (resp.get() == nullptr) {
        std::cerr << "Sender::worker_thread(): received nullptr instead of response, "
                  << "looks like it's a bug" << std::endl;
        continue;
      }

      if (!resp->succeeded()) {
        std::cerr << "Sender::worker_thread(): response was not successfully formed "
                  << "(UID=" << resp->context()->id() << ")" << std::endl;
        continue;
      }

      std::unique_ptr<char[]> data;
      size_t size = 0;
      std::stringstream errors;
      if (!resp->serialize(&data, &size, &errors)) {
        std::cerr << "Sender::worker_thread(): failed to serialize response (UID=" << resp->context()->id()
                  << "): " << errors.str().c_str() << std::endl;
        size = 0;
      }

      if (size != 0) {
        if (!swm_write_exact(output_, data.get(), size)) {
          std::cerr << "Sender::worker_thread(): failed to send serialized response data (UID=" << resp->context()->id()
                    << ")" << std::endl;
        }
      }
    } catch (std::exception &ex) {
      std::cerr << "Exception from Sender::worker_thread(): " << ex.what() << std::endl;
    }
  }
}

}  // namespace util
}  // namespace swm
