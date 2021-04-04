
#include "timetable_info.h"

TimetableInfo::TimetableInfo(const TimetableInfoInterface *table) {
  if (table != nullptr && !table->empty()) {
    const auto &tts = table->tables();
    tables_.resize(tts.size());
    tables_refs_.resize(tts.size());
    for (size_t i = 0; i < tts.size(); ++i) {
      tables_[i] = *tts[i];
      tables_refs_[i] = &tables_[i];
    }
  }
}

TimetableInfo::TimetableInfo(std::vector<swm::SwmTimetable> *table_content) {
  if (table_content != nullptr && !table_content->empty()) {
    tables_ = std::move(*table_content);
    tables_refs_.resize(tables_.size());
    for (size_t i = 0; i < tables_.size(); ++i) {
      tables_refs_[i] = &tables_[i];
    }
  }
}
