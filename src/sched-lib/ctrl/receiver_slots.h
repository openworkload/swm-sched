
#pragma once

#include <cstdint>
#include <memory>

namespace swm {
namespace util {

// Protocol slot count is a single wire byte. Cap to limit resize DoS.
constexpr uint8_t kMaxReceiverDataSlots = 64;

// Per-slice ETF payload size (wire uint32 length). Matches swm-core ETF order.
constexpr uint32_t kMaxReceiverSliceBytes = 16u * 1024u * 1024u;

// Max EI list/map element count accepted while decoding schedule payloads.
constexpr int kMaxEiListElements = 1000000;

// One wire data slice: ETF bytes plus known size for bound checks.
struct ReceiverSlice {
  std::unique_ptr<char[]> data;
  uint32_t size = 0;
};

// Return true when ``type`` is a valid index into a vector of size ``total``.
inline bool receiver_slot_ok(uint8_t type, uint8_t total) {
  return type < total;
}

inline bool receiver_total_ok(uint8_t total) {
  return total <= kMaxReceiverDataSlots;
}

// Reject zero and oversized slice lengths before allocation.
inline bool receiver_slice_len_ok(uint32_t len) {
  return len > 0 && len <= kMaxReceiverSliceBytes;
}

}  // namespace util
}  // namespace swm
