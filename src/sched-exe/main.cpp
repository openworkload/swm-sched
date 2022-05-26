
#include <fstream>
#include <iostream>

#include "hw/scanner.h"
#include "alg/algorithm_factory.h"
#include "ctrl/service.h"
#include "cli_args.h"

int main(int argc, char* const argv[]) {
  size_t ivalue;
  double dvalue;
  std::string svalue;
  std::stringstream errors;

  try {
    // Parsing Command Line Arguments
    swm::CliArgs args;
    if (!args.init(argc, argv, &errors)) {
      std::cerr << "Wrong command line options: " << errors.str() << std::endl;
      return -1;
    }

    // Processing --help flag
    if (args.has_help_flag()) {
      swm::CliArgs::format_help_message(&std::cout);
      std::cout << std::endl;
      return 0;
    }

    // Constructing factories
    swm::Scanner scanner;
    if (!scanner.scan()) {
      std::cerr << "Failed to detect available computing units" << std::endl;
      return -1;
    }
    swm::AlgorithmFactory factory;
    if (!factory.load_plugins(args.has_plugins_flag(&svalue) ? svalue : ".", &errors)) {
      std::cerr << "Failed to load plugins: " << errors.str() << std::endl;
      return -2;
    }

    if (ei_init()) {
      std::cerr << "Can't initialize ei library" << std::endl;
      return -3;
    }

    // Configuring service
    swm::Service service(&factory, &scanner);
    if (args.has_debug_flag()) {
      service.set_debug_mode(true);
    }
    if (args.has_in_queue_flag(&ivalue)) {
      service.set_in_queue_size(ivalue);
    }
    if (args.has_out_queue_flag(&ivalue)) {
      service.set_out_queue_size(ivalue);
    }
    if (args.has_timeout_flag(&dvalue)) {
      service.set_timeout(dvalue);
    }
    
    std::ifstream input;
    if (args.has_input_flag(&svalue)) {
      input.open(svalue.c_str());
      if (!input.good()) {
        std::cerr << "Failed to open file with input commands" << std::endl;
        return -3;
      }
      service.set_input(&input);
    }

    // Done! Starting service, current thread will be blocked
    service.main_loop();
    return 0;
  }
  catch (std::exception &err) {
    std::cerr << "Exception thrown: " << err.what() << std::endl;
    return -42;
  }
}
