
#include "lib_binding.h"

extern "C" {
namespace swm {
namespace util {

LibBinding::LibBinding(const std::string &lib_loc,
                       const std::function<std::string()> &free_lib_func,
                       const std::shared_ptr<AlgorithmDescInterface> &alg_desc,
                       void *create_ctx_ptr, void *release_ctx_ptr,
                       void *construct_tt_ptr, void *improve_tt_ptr, void *bind_cu_ptr)
    : lib_location_(lib_loc),
      free_lib_func_(free_lib_func),
      alg_desc_(alg_desc),
      create_ctx_func_((CreateContextFunc)create_ctx_ptr),
      release_ctx_func_((ReleaseContextFunc)release_ctx_ptr),
      construct_timetable_func_((ConstructTimetableFunc)construct_tt_ptr),
      improve_timetable_func_((ImproveTimetableFunc)improve_tt_ptr),
      bind_func_((BindComputeUnitFunc)bind_cu_ptr) {
  
  if (alg_desc == nullptr) {
    throw std::runtime_error(
      "LibBinding::LibBinding(): algorithm's descriptor cannot be equal to nullptr");
  }

  if (create_ctx_ptr == nullptr || release_ctx_ptr == nullptr ||
      construct_tt_ptr == nullptr || improve_tt_ptr == nullptr || bind_cu_ptr == nullptr) {
    throw std::runtime_error(
      "LibBinding::LibBinding(): dynamic functions cannot be equal to nullptr");
  }
}

LibBinding::~LibBinding() {
  alg_desc_.reset();             // deleting object from dll/so BEFORE its destructor unloaded
  auto error = free_lib_func_();
  
  if (!error.empty()) {
    // Whoops! Failed to unload dynamic library. But we can do nothing!
    // At least, let's tell system administrator about it.
    std::cerr << "Failed to unload dynamic library " << lib_location_.c_str()
              << ", details: " << error.c_str() << std::endl;
  }
}

static inline bool safe_call(const std::function<bool(std::stringstream *)> &func,
  std::stringstream *error) {
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }

  try { return func(error); }
  catch (std::exception &ex) {
    *error << "an exception has been thrown by plugin: " << ex.what();
    return false;
  }
}

bool LibBinding::create_context(MetricsInterface *metrics, void **ctx,
                                std::stringstream *error) const {
  if (ctx == nullptr) {
    throw std::runtime_error(
      "LibBinding::create_context(): context's pointer cannot be equal to nullptr");
  }

  auto func = [metrics, ctx, f = create_ctx_func_](std::stringstream *error) -> bool {
    return f(metrics, ctx, error);
  };
  return safe_call(func, error);
}

bool LibBinding::release_context(void *ctx, std::stringstream *error) const {
  auto func = [ctx, f = release_ctx_func_](std::stringstream *error) -> bool {
    return f(ctx, error);
  };
  return safe_call(func, error);
}

bool LibBinding::construct_timetable(void *ctx, const SchedulingInfoInterface *info,
                                     PluginEventsInterface *events,
                                     std::shared_ptr<swm::TimetableInfoInterface> *table,
                                     std::stringstream *error) const {
  if (info == nullptr || table == nullptr) {
    throw std::runtime_error(
      "LibBinding::construct_timetable(): info and table cannot be equal to nullptr");
  }
  auto func = [ctx, info, events, table,
               f = construct_timetable_func_](std::stringstream *error) -> bool {
    return f(ctx, info, events, table, error);
  };
  return safe_call(func, error);
}

bool LibBinding::improve_timetable(void *ctx,
                                   const TimetableInfoInterface *old_table,
                                   PluginEventsInterface *events,
                                   std::shared_ptr<swm::TimetableInfoInterface> *new_table,
                                   std::stringstream *error) const {
  if (old_table == nullptr || new_table == nullptr) {
    throw std::runtime_error(
      "LibBinding::improve_timetable(): info and table cannot be equal to nullptr");
  }
  auto func = [ctx, old_table, events, new_table,
    f = improve_timetable_func_](std::stringstream *error) -> bool {
    return f(ctx, old_table, events, new_table, error);
  };
  return safe_call(func, error);
}

bool LibBinding::bind_to_compute_unit(void *ctx, const ComputeUnitInterface *cu,
                                      std::stringstream *error) const {
  if (cu == nullptr) {
    throw std::runtime_error(
      "LibBinding::bind_to_compute_unit(): compute unit cannot be defined by nullptr");
  }
  auto func = [ctx, cu, f = bind_func_](std::stringstream *error) -> bool {
    return f(ctx, cu, error);
  };
  return safe_call(func, error);
}

} // util
} // swm
} // extern "C"
