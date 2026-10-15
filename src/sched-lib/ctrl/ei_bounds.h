
#pragma once

#include "receiver_slots.h"

#include <sstream>

namespace swm {
namespace util {

// True when EI byte index is inside [0, buf_size].
inline bool ei_index_in_bounds(int index, uint32_t buf_size) {
  return index >= 0 && static_cast<uint32_t>(index) <= buf_size;
}

inline bool ei_list_size_ok(int list_size) {
  return list_size >= 0 && list_size <= kMaxEiListElements;
}

// After an ei_* call that advances index: require still in-bounds.
inline bool ei_require_index(int index, uint32_t buf_size, std::stringstream *errors, const char *what) {
  if (!ei_index_in_bounds(index, buf_size)) {
    if (errors != nullptr) {
      *errors << what << ": EI index " << index << " outside buffer size " << buf_size;
    }
    return false;
  }
  return true;
}

inline bool ei_require_list_size(int list_size, std::stringstream *errors, const char *what) {
  if (!ei_list_size_ok(list_size)) {
    if (errors != nullptr) {
      *errors << what << ": EI list size " << list_size << " exceeds max " << kMaxEiListElements;
    }
    return false;
  }
  return true;
}

}  // namespace util
}  // namespace swm
