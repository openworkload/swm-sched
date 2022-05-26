
#include "responses.h"

#include "constants.h"
#include "chn/metrics_snapshot.h"


namespace swm {
namespace util {

//-------------------------
//--- ResponseInterface ---
//-------------------------

void ResponseInterface::refresh_timers() {
  double astro = 0.0;
  double idling = 0.0;
  double working = 0.0;
  context()->timer()->get_times(&astro, &idling, &working);
  result_.set_astro_time(astro);
  result_.set_idle_time(idling);
  result_.set_work_time(working);
}

ei_x_buff ResponseInterface::make_timetables_ei_buffer(
    const std::vector<SwmTimetable> &timetables,
    std::stringstream *errors) const {

  ei_x_buff x;
  if (ei_x_new(&x)) {
    *errors << "Can't create new ei_x_buff for timetables" << std::endl;
    return x;
  }
  if (ei_x_encode_list_header(&x, timetables.size())) {
    *errors << "Can't create timetables: can't encode list header" << std::endl;
    ei_x_free(&x);
    return x;
  }

  for (const auto& table: timetables) {
    const auto &nodes = table.get_job_nodes();
    if (nodes.empty()) {
      continue;
    }
    if (ei_x_encode_tuple_header(&x, 4)) {
      *errors << "Can't create timetable: can't encode tuple header" << std::endl;
      ei_x_free(&x);
      return x;
    }
    if (ei_x_encode_atom(&x, "timetable")) {
      *errors << "Can't create timetable: can't encode first atom" << std::endl;
      ei_x_free(&x);
      return x;
    }
    if (ei_x_encode_ulong(&x, table.get_start_time())) {
      *errors << "Can't create timetable: can't encode start time" << std::endl;
      ei_x_free(&x);
      return x;
    }
    if (ei_x_encode_string(&x, table.get_job_id().c_str())) {
      *errors << "Can't create timetable: can't encode job id" << std::endl;
      ei_x_free(&x);
      return x;
    }

    const size_t nodes_cnt = nodes.size();
    if (ei_x_encode_list_header(&x, nodes_cnt)) {
      *errors << "Can't create timetable: can't encode nodes list header" << std::endl;
      ei_x_free(&x);
      return x;
    }
    for (size_t i = 0; i < nodes_cnt; ++i) {
      if (ei_x_encode_string(&x, nodes[nodes_cnt - i - 1].c_str())) {
        ei_x_free(&x);
        return x;
      }
    }
    if (nodes_cnt && ei_x_encode_empty_list(&x)) {
      *errors << "Can't create timetable: can't encode last nodes list element" << std::endl;
      ei_x_free(&x);
      return x;
    }
  }

  if (timetables.size() && ei_x_encode_empty_list(&x)) {
    *errors << "Can't create timetables list: can't encode last element" << std::endl;
    ei_x_free(&x);
  }

  return x;
}

ei_x_buff ResponseInterface::make_metrics_ei_buffer(
    const std::vector<SwmMetric> &metrics,
    std::stringstream *errors) const {

  ei_x_buff x;
  if (ei_x_new(&x)) {
    *errors << "Can't create new ei_x_buff for metrics" << std::endl;
    return x;
  }
  if (ei_x_encode_list_header(&x, metrics.size())) {
    *errors << "Can't create metrics: can't encode list header" << std::endl;
    ei_x_free(&x);
    return x;
  }

  for (const auto& metric: metrics) {
    if (ei_x_encode_tuple_header(&x, 4)) {
      *errors << "Can't create metric: can't encode tuple header" << std::endl;
      ei_x_free(&x);
      return x;
    }
    if (ei_x_encode_atom(&x, "metric")) {
      *errors << "Can't create metric: can't encode first atom" << std::endl;
      ei_x_free(&x);
      return x;
    }
    if (ei_x_encode_atom(&x, metric.get_name().c_str())) {
      *errors << "Can't create metric: can't encode name" << std::endl;
      ei_x_free(&x);
      return x;
    }
    if (ei_x_encode_ulong(&x, metric.get_value_integer())) {
      *errors << "Can't create metric: can't encode value integer" << std::endl;
      ei_x_free(&x);
      return x;
    }
    if (ei_x_encode_double(&x, metric.get_value_float64())) {
      *errors << "Can't create metric: can't encode value float64" << std::endl;
      ei_x_free(&x);
      return x;
    }
  }

  if (metrics.size() && ei_x_encode_empty_list(&x)) {
    *errors << "Can't create metrics list: can't encode last element" << std::endl;
    ei_x_free(&x);
  }

  return x;
}

ei_x_buff ResponseInterface::make_scheduler_result_ei_buffer(const std::vector<SwmTimetable> &timetables,
                                                             const std::vector<SwmMetric> &metrics,
                                                             std::stringstream *errors) const {
  ei_x_buff timetables_buff = make_timetables_ei_buffer(timetables, errors);
  ei_x_buff metrics_buff  = make_metrics_ei_buffer(metrics, errors);

  ei_x_buff x;
  if (ei_x_new(&x)) {
    *errors << "Can't create new ei_x_buff" << std::endl;
    return x;
  }
  if (ei_x_encode_version(&x)) {
    *errors << "Can't encode binary format version" << std::endl;
    ei_x_free(&x);
    return x;
  }
  if (ei_x_encode_tuple_header(&x, 8)) {
    *errors << "Can't create scheduler result: can't encode tuple header" << std::endl;
    ei_x_free(&x);
    return x;
  }
  if (ei_x_encode_atom(&x, "scheduler_result")) {
    *errors << "Can't create scheduler result: can't encode atom scheduler_result" << std::endl;
    ei_x_free(&x);
    return x;
  }
  if (ei_x_append(&x, &timetables_buff)) {
    *errors << "Can't create scheduler result: can't encode timetables" << std::endl;
    ei_x_free(&x);
    return x;
  }
  if (ei_x_append(&x, &metrics_buff)) {
    *errors << "Can't create scheduler result: can't encode metrics" << std::endl;
    ei_x_free(&x);
    return x;
  }
  if (ei_x_encode_string(&x, result_.get_request_id().c_str())) {
    *errors << "Can't create scheduler result: can't encode request ID" << std::endl;
    ei_x_free(&x);
    return x;
  }
  if (ei_x_encode_long(&x, result_.get_status())) {
    *errors << "Can't create scheduler result: can't encode status" << std::endl;
    ei_x_free(&x);
    return x;
  }
  if (ei_x_encode_double(&x, result_.get_astro_time())) {
    *errors << "Can't create scheduler result: can't encode astro time" << std::endl;
    ei_x_free(&x);
    return x;
  }
  if (ei_x_encode_double(&x, result_.get_idle_time())) {
    *errors << "Can't create scheduler result: can't encode idle time" << std::endl;
    ei_x_free(&x);
    return x;
  }
  if (ei_x_encode_double(&x, result_.get_work_time())) {
    *errors << "Can't create scheduler result: can't encode work time" << std::endl;
    ei_x_free(&x);
    return x;
  }

  return x;
}

//-------------------------
//--- TimetableResponse ---
//-------------------------

TimetableResponse::TimetableResponse(const std::shared_ptr<CommandContext> &context,
                                     const std::shared_ptr<swm::TimetableInfoInterface> &tables_info,
                                     const std::shared_ptr<MetricsSnapshot> &metrics)
      : context_(context), metrics_(metrics) {
  result_.set_request_id(context_->id());

  const auto ttp = tables_info->tables();
  std::vector<SwmTimetable> tt;
  tt.reserve(ttp.size());
  for(const auto &t: ttp) {
    tt.push_back(*t);
  }
  result_.set_timetable(tt);
  result_.set_status(succeeded());
  result_.print("   ", '\n');
}

bool TimetableResponse::serialize(std::unique_ptr<char[]> *data,
                                  size_t *size,
                                  std::stringstream *errors) {
  if (data == nullptr || size == nullptr) {
    throw std::runtime_error(
      "TimetableResponse::serialize(): \"data\" and \"size\" cannot be equal to nullptr");
  }
  std::stringstream errors_;
  if (errors == nullptr) {
    errors = &errors_;
  }
  refresh_timers();
  const ei_x_buff x = make_scheduler_result_ei_buffer(result_.get_timetable(), result_.get_metrics(), errors);

  data->reset(x.buff);
  *size = x.index;
  return x.buff != nullptr;
}

//-----------------------
//--- MetricsResponse ---
//-----------------------

bool MetricsResponse::serialize(std::unique_ptr<char[]> *data, size_t *size, std::stringstream *) {
  if (data == nullptr || size == nullptr) {
    throw std::runtime_error("MetricsResponse::serialize(): \"data\" and \"size\" cannot be equal to nullptr");
  }

  //TODO: we need to serialize request with metrics only
  //auto alg_metrics = metrics_->algorithm_metrics(); // per algorithm metrics
  //auto chain_metrics = metrics_->chain_metrics();   // chain metrics
  //auto serv_metrics = metrics_->service_metrics();  // global service metrics

  // Enumeration can be implemented as:
  //auto en = chain_metrics.int_value_indices();
  //for (const auto rec : en) {
  //  auto ival = chain_metrics.int_value(rec.first);
  //  auto name = rec.second;
  //}
  // And the same for double values (if needed)

  data->reset(nullptr);
  *size = 0;
  return true;
}

//---------------------
//--- EmptyResponse ---
//---------------------

bool EmptyResponse::serialize(std::unique_ptr<char[]> *data,
                              size_t *size,
                              std::stringstream *errors) {
  if (data == nullptr || size == nullptr) {
    throw std::runtime_error(
      "EmptyResponse::serialize(): \"data\" and \"size\" cannot be equal to nullptr");
  }
  const ei_x_buff x =  make_scheduler_result_ei_buffer({}, {}, errors);
  data->reset(x.buff);
  *size = x.index;
  return x.buff != nullptr;
}

} // util
} // swm
