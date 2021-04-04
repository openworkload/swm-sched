
#pragma once

#include "plugin_defs.h"

class TimetableInfo : public swm::TimetableInfoInterface {
 public:
  TimetableInfo() = default;
  TimetableInfo(const TimetableInfo &) = delete;
  void operator =(const TimetableInfo &) = delete;

  TimetableInfo(const TimetableInfoInterface *table);
  TimetableInfo(std::vector<swm::SwmTimetable> *table_content);
  virtual const std::vector<const swm::SwmTimetable *> &tables() const override {
    return tables_refs_;
  }
  virtual bool empty() const override { return tables_refs_.empty(); }

 private:
  std::vector<swm::SwmTimetable> tables_;
  std::vector<const swm::SwmTimetable *> tables_refs_;
};

