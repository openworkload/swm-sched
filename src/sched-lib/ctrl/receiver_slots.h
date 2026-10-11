
#pragma once

#include <cstdint>

namespace swm {
namespace util {

// Protocol slot count is a single wire byte. Cap to limit resize DoS.
constexpr uint8_t kMaxReceiverDataSlots = 64;

// Return true when ``type`` is a valid index into a vector of size ``total``.
inline bool receiver_slot_ok(uint8_t type, uint8_t total) {
  return type < total;
}

inline bool receiver_total_ok(uint8_t total) {
  return total <= kMaxReceiverDataSlots;
}

}  // namespace util
}  // namespace swm
