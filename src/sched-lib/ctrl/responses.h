
#pragma once

#include "defs.h"
#include "command_context.h"
#include "ifaces/timetable_info_interface.h"

#include "wm_scheduler_result.h"

namespace swm {
namespace util {

class Sender;
class MetricsSnapshot;

class ResponseInterface {
 public:
  ResponseInterface(const ResponseInterface &) = delete;
  virtual ~ResponseInterface() { }
  void operator =(const ResponseInterface &) = delete;

  virtual const std::shared_ptr<CommandContext> &context() const = 0;
  virtual bool succeeded() const = 0;

 protected:
  ResponseInterface() { };
  virtual bool serialize(std::unique_ptr<char[]> *data, size_t *size,
                         std::stringstream *errors) = 0;
  ei_x_buff make_scheduler_result_ei_buffer(const std::vector<SwmTimetable> &timetables,
                                            const std::vector<SwmMetric> &metrics,
                                            std::stringstream *errors) const;
  bool encode_scheduler_result(const char* result_eterm, size_t *size,
                               std::unique_ptr<char[]> *data,
                               std::stringstream *errors) const;
  void refresh_timers();

  SwmSchedulerResult result_;

 friend class Sender;

 private:
  ei_x_buff make_timetables_ei_buffer(const std::vector<SwmTimetable> &timetables, std::stringstream *errors) const;
  ei_x_buff make_metrics_ei_buffer(const std::vector<SwmMetric> &metrics, std::stringstream *errors) const;
};


class TimetableResponse : public ResponseInterface {
 public:
  TimetableResponse(const std::shared_ptr<CommandContext> &context,
                    const std::shared_ptr<swm::TimetableInfoInterface>&,
                    const std::shared_ptr<MetricsSnapshot> &metrics);

  virtual const std::shared_ptr<CommandContext> &context() const override { return context_; }
  virtual bool succeeded() const override { return true; }

 private:
  virtual bool serialize(std::unique_ptr<char[]> *data,
                         size_t *size, std::stringstream *errors) override;

  std::shared_ptr<CommandContext> context_;
  std::shared_ptr<MetricsSnapshot> metrics_;
};

class MetricsResponse : public ResponseInterface {
 public:
  MetricsResponse(const std::shared_ptr<CommandContext> &context,
                  const std::shared_ptr<MetricsSnapshot> &metrics)
      : context_(context), metrics_(metrics) { }

  virtual const std::shared_ptr<CommandContext> &context() const override { return context_; }
  virtual bool succeeded() const override { return true; };

 private:
  virtual bool serialize(std::unique_ptr<char[]> *data, size_t *size,
                         std::stringstream *errors) override;
  std::shared_ptr<CommandContext> context_;
  std::shared_ptr<MetricsSnapshot> metrics_;
};


class EmptyResponse : public ResponseInterface {
 public:
  EmptyResponse(const std::shared_ptr<CommandContext> &context,
                bool is_succeeded)
      : context_(context), succeeded_(is_succeeded) { }

  virtual const std::shared_ptr<CommandContext> &context() const override { return context_; }
  virtual bool succeeded() const override { return succeeded_; };

 private:
  virtual bool serialize(std::unique_ptr<char[]> *data, size_t *size,
                         std::stringstream *errors) override;
  std::shared_ptr<CommandContext> context_;
  bool succeeded_;
};

} // util
} // swm
