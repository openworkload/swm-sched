
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

ETERM* ResponseInterface::make_scheduler_result_eterm(std::vector<ETERM*> timetable_eterms,
                                                      std::vector<ETERM*> metric_eterms) const {
  ETERM *props[SwmSchedulerResultTupleSize];
  props[0] = erl_mk_atom("scheduler_result");

  props[1] = erl_mk_empty_list();
  for (const auto x: timetable_eterms) {
    props[1] = erl_cons(x, props[1]);
  }

  props[2] = erl_mk_empty_list();
  for (const auto x: metric_eterms) {
    props[2] = erl_cons(x, props[2]);
  }

  props[3] = erl_mk_string(result_.get_request_id().c_str());
  props[4] = erl_mk_ulonglong(result_.get_status());
  props[5] = erl_mk_float(result_.get_astro_time());
  props[6] = erl_mk_float(result_.get_idle_time());
  props[7] = erl_mk_float(result_.get_work_time());

  return erl_mk_tuple(props, SwmSchedulerResultTupleSize);
}

bool ResponseInterface::encode_scheduler_result(ETERM* result_eterm,
                                                size_t* size,
                                                std::unique_ptr<unsigned char[]> *data,
                                                std::stringstream *errors) const {
  const size_t len = erl_term_len(result_eterm);
  if (len) {
    data->reset(new unsigned char[len]);
    *size = erl_encode(result_eterm, data->get());
    erl_free_compound(result_eterm);
  } else {
    *size = 1;
    data->reset(new unsigned char[*size]);
    *data->get() = 0;
  }

  //TODO: serialize metrics as well. See MetricsResponse::serialize() for details
  //auto m = metrics_;
  
  if (*size == 0) {
    *errors << "cannot encode ETERM";
    return false;
  }
  return true;
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
  //result_.print("   ", '\n');
}

bool TimetableResponse::serialize(std::unique_ptr<unsigned char[]> *data,
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
  const auto timetable_eterms = make_timetable_eterms();
  const auto metric_eterms = make_metric_eterms();
  const auto result_eterm = make_scheduler_result_eterm(timetable_eterms, metric_eterms);
  const bool is_encoded = encode_scheduler_result(result_eterm, size, data, errors);
  return is_encoded;
}

std::vector<ETERM*> TimetableResponse::make_timetable_eterms() const {
  std::vector<ETERM*> timetable_eterms;
  for (const auto& table: result_.get_timetable()) {
    const auto &nodes = table.get_job_nodes();
    if (nodes.empty()) {
      continue;
    }

    ETERM *props[SwmTimeTableTupleSize];
    props[0] = erl_mk_atom("timetable");
    props[1] = erl_mk_ulonglong(table.get_start_time());
    props[2] = erl_mk_string(table.get_job_id().c_str());

    props[3] = erl_mk_empty_list();
    size_t nodes_cnt = nodes.size();
    for (size_t j = 0; j < nodes_cnt; ++j) {
      props[3] = erl_cons(erl_mk_string(nodes[nodes_cnt - j - 1].c_str()), props[3]);
    }

    ETERM *eterm = erl_mk_tuple(props, SwmTimeTableTupleSize);
    timetable_eterms.push_back(eterm);
  }
  return timetable_eterms;
}

std::vector<ETERM*> TimetableResponse::make_metric_eterms() const {
  std::vector<ETERM*> metric_eterms;
  for (const auto& metric: result_.get_metrics()) {
    ETERM *props[SwmMetricTupleSize];
    props[0] = erl_mk_atom("metric");
    props[1] = erl_mk_string(metric.get_name().c_str());
    props[2] = erl_mk_ulonglong(metric.get_value_integer());
    props[3] = erl_mk_float(metric.get_value_float64());
    ETERM *eterm = erl_mk_tuple(props, SwmMetricTupleSize);
    metric_eterms.push_back(eterm);
  }
  return metric_eterms;
}

//-----------------------
//--- MetricsResponse ---
//-----------------------

bool MetricsResponse::serialize(std::unique_ptr<unsigned char[]> *data, size_t *size,
                                std::stringstream *) {
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

bool EmptyResponse::serialize(std::unique_ptr<unsigned char[]> *data, size_t *size,
                              std::stringstream *)
{
  if (data == nullptr || size == nullptr) {
    throw std::runtime_error(
      "EmptyResponse::serialize(): \"data\" and \"size\" cannot be equal to nullptr");
  }

  std::stringstream errors;
  const auto result_eterm = make_scheduler_result_eterm({}, {});
  const bool is_encoded = encode_scheduler_result(result_eterm, size, data, &errors);
  if (errors.str().size()) {
    std::cerr << "ERRORS: " << errors.str() << std::endl;
  }
  return is_encoded;
}

} // util
} // swm
