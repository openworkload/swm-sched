
#include "receiver.h"

namespace swm {
namespace util {

Receiver::~Receiver() {
  closed_ = true;
  if (worker_.joinable()) {
    worker_.join();
  }
}

void Receiver::init(MyQueue<std::shared_ptr<CommandInterface> > *queue, std::istream *input) {
  if (queue_ != nullptr || input_ != nullptr) {
    throw std::runtime_error("Receiver::init(): object was already initialized");
  }
  if (queue == nullptr || input == nullptr) {
    throw std::runtime_error(
      "Receiver::init(): \"queue\" and \"input\" cannot be equal to nullptr");
  }

  if (worker_.joinable()) {
    worker_.join();
  }

  input_ = input;
  queue_ = queue;
  closed_ = false;
  finished_ = false;
  worker_ = std::thread([me = this]() -> void { me->worker_loop(); });
}

bool Receiver::finished() {
  if (queue_ == nullptr || input_ == nullptr) {
    throw std::runtime_error("Receiver::init(): object must be initialized first");
  }

  return finished_;
}

bool Receiver::get_data(std::vector<std::unique_ptr<char[]>> *data,
                        CommandType *cmd,
                        SwmUID *uid,
                        std::stringstream *errors) {
  if (data == nullptr || cmd == nullptr || uid == nullptr) {
    throw std::runtime_error(
      "Receiver::get_data(): \"data\", \"cmd\", \"uid\" cannot be equal to nullptr");
  }
  data->clear();

  std::stringstream errors_;
  if (errors == nullptr) {
    errors = &errors_;
  }

  char command;
  if (!swm_read_exact(input_, &command, 1)) {
    *errors << "could not read command";
    return false;
  }
  if (command != SWM_COMMAND_SCHEDULE && command != SWM_COMMAND_INTERRUPT &&
      command != SWM_COMMAND_METRICS  && command != SWM_COMMAND_EXCHANGE) {
    *errors << "unknown command #" << int(command);
    return false;
  }
  *cmd = (CommandType)command;

  //TODO: read uid
  *uid = 1;

  char total = 0;
  if (!swm_read_exact(input_, &total, 1)) {
    *errors << "could not read total data count";
    return false;
  }
  
  data->resize(total);
  for (unsigned char i = 0; i < total; ++i) {
    char type = 0;
    if (!swm_read_exact(input_, &type, 1)) {
      *errors << "could not read data type (i=" << i << ")";
      return false;
    }
    if (type >= total) {
      *errors << "wrong data type: " << type;
      return false;
    }

    uint32_t len = 0;
    if(!swm_read_length(input_, &len)) {
      *errors << "data length is 0 (type=" << type << ")";
      return false;
    }

    (*data)[type].reset(new char[len]);
    auto ptr = (*data)[type].get();
    if (!swm_read_exact(input_, ptr, len)) {
      *errors << "couldn't get " << len << " bytes of data type " << type;
      return false;
    }
  }

  return true;
}

static inline bool is_good(std::istream *str) {
  if (str != nullptr) {
    str->peek();
    return str->good();
  }
  return false;
}

void Receiver::worker_loop() {
  std::vector<std::unique_ptr<char[]>> data;
  CommandType cmd_type;
  SwmUID uid;
  std::stringstream errors;

  while (is_good(input_) && !closed_) {
    data.clear();
    errors.str("");

    // The first stage - to read raw data
    // We cannot recover stream after any error
    try {
      if (!get_data(&data, &cmd_type, &uid, &errors)) {
        std::cerr << "Receiver::worker_loop(): failed to read command's data, details: "
                  << errors.str() << std::endl;
        break;
      }
    }
    catch (std::runtime_error &ex) {
      std::cerr << "Exception from Receiver::worker_loop(): " << ex.what() << ". "
                << "Aborting." << std::endl;
    }

    // The second stage - to parse raw data
    // Now, we can handle errors
    try {
      std::shared_ptr<CommandInterface> command = nullptr;
      std::shared_ptr<CommandContext> context(new CommandContext(uid));
      context->timer()->turn_on();
      switch (cmd_type) {
        case SWM_COMMAND_SCHEDULE: {
          command.reset(new ScheduleCommand(context));
          break;
        }
        case SWM_COMMAND_INTERRUPT: {
          command.reset(new InterruptCommand(context));
          break;
        }
        case SWM_COMMAND_METRICS: {
          command.reset(new MetricsCommand(context));
          break;
        }
        case SWM_COMMAND_EXCHANGE: {
          command.reset(new ExchangeCommand(context));
          break;
        }
        default: {
          std::cerr << "Receiver::worker_loop(): received unknown command (UID="
                    << uid << ", type=#" << (int)cmd_type << "), ignoring it." << std::endl;
          command.reset(new CorruptedCommand(context));
        }
      }

      if (!command->init(data, &errors)) {
        std::cerr << "Receiver::worker_loop(): failed to parse command's data (UID="
                  << uid << "), ignoring it." << std::endl;
        std::cerr << "Errors: " << errors.str() << std::endl;
        command.reset(new CorruptedCommand(context));
      }

      context->timer()->turn_off();
      queue_->push(command);
    }
    catch (std::runtime_error &ex) {
      std::cerr << "Exception from Receiver::worker_loop(): " << ex.what() << ". "
                << "Ignoring corrupted command." << std::endl;
    }
  }

  finished_ = true;
}

} // util
} // swm
