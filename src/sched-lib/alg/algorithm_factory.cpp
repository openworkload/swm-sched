
#include "algorithm_factory.h"

#include <algorithm>

#include "auxl/file.h"
#include "auxl/directory.h"
#include "auxl/lib_funcs.h"


namespace swm {

bool AlgorithmFactory::load_plugins(const std::string &path, std::stringstream *errors) {
  std::stringstream errors_;
  if (errors == nullptr) {
    errors = &errors_;
  }

  if (!alg_ptrs_.empty()) {
    *errors << "plugins have been already loaded";
    return false;
  }

  // Getting potential candidates for plugins.
  std::vector<std::string> files;
  try {
    util::find_files(path, std::string("*") + util::get_library_extension(), &files);
  }
  catch (std::runtime_error &err) {
    *errors << "failed to find plugin files (" << err.what() << ")";
    return false;
  }

  if (files.empty()) {
    *errors << "no plugins were found";
    return false;
  }

  // Loading all plugins. Failed to load one - just printing error to error-stream.
  for (size_t i = 0; i < files.size(); i++) {
    void *lib = nullptr;
    try {
      if ((lib = util::load_library(files[i])) != nullptr) {
        void *cc_ptr
          = util::get_library_function(lib, util::LibBinding::create_context_name());
        void *rc_ptr
          = util::get_library_function(lib, util::LibBinding::release_context_name());
        void *construct_tt_ptr
          = util::get_library_function(lib, util::LibBinding::construct_timetable_name());
        void *improve_tt_ptr
          = util::get_library_function(lib, util::LibBinding::improve_timetable_name());
        void *cu_ptr
          = util::get_library_function(lib, util::LibBinding::bind_compute_unit_name());
        void *ad_ptr
          = util::get_library_function(lib, util::LibBinding::create_algorithm_desc_name());

        if (cc_ptr == nullptr || rc_ptr == nullptr ||
            construct_tt_ptr == nullptr || improve_tt_ptr == nullptr ||
            cu_ptr == nullptr || ad_ptr == nullptr) {
          throw std::runtime_error("cannot load all functions");
        }

        std::shared_ptr<AlgorithmDescInterface> desc;
        ((util::LibBinding::CreateAlgorithmDescFunc)ad_ptr)(&desc);
        if (desc.get() == nullptr) {
          throw std::runtime_error("plugin's descriptor is equal to nullptr");
        }

        auto free_func = [lib]() -> std::string {
          return util::free_library(lib) ? std::string() : util::get_library_error();
        };
        auto binding = std::shared_ptr<util::LibBinding>(
                                   new util::LibBinding(util::file_full_path(files[i]),
                                                        free_func, desc,
                                                        cc_ptr, rc_ptr,
                                                        construct_tt_ptr, improve_tt_ptr, cu_ptr));
        lib_bindings_.push_back(binding);
        alg_ptrs_.push_back(binding->get_algorithm_descriptor());
      } else {
        std::cerr << "Failed to load library " << files[i].c_str() << std::endl;
      }
    }
    catch (std::exception &err) {
      std::cerr << "Failed to load plugin " << files[i].c_str() << ", details: " << err.what() << std::endl;
      if (!util::free_library(lib)) {
        std::cerr << "Failed to unload library " << files[i].c_str() << std::endl;
      }
    }
  }

  // Ensuring that at least one plugin was loaded successfully.
  if (alg_ptrs_.empty()) {
    *errors << "Failed to load at least one plugin";
    lib_bindings_.clear();
    return false;
  }

  return true;
}

 bool AlgorithmFactory::create(const AlgorithmDescInterface *desc,
                               std::shared_ptr<Algorithm> *res,
                               std::stringstream *error) const {
  if (res == nullptr) {
    throw std::runtime_error(
      "AlgorithmFactory::create(): pointer for result cannot be equal to nullptr");
  }
  if (desc == nullptr) {
    throw std::runtime_error(
      "AlgorithmFactory::create(): algorithm's descriptor cannot be equal to nullptr");
  }

  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }

  auto pred = [desc](const std::shared_ptr<util::LibBinding> &value) {
    return value->get_algorithm_descriptor() == desc;
  };
  auto itr = std::find_if(lib_bindings_.begin(), lib_bindings_.end(), pred);

  if (itr == lib_bindings_.end()) {
    *error << "failed to determine plugin by its descriptor";
    return false;
  }

  res->reset(new Algorithm(*itr));
  if (!(*res)->init(error)) {
    return false;
  }

  return true;
}

} // swm
