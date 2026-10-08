#ifndef TRACE_CHECKSUM_H
#define TRACE_CHECKSUM_H
#include <stddef.h>
#include <stdint.h>
inline uint32_t traceChecksum(const char* data, size_t length) {
  uint32_t hash=2166136261UL;
  for (size_t i=0;i<length;++i) {
    hash^=static_cast<uint8_t>(data[i]);
    hash*=16777619UL;
  }
  return hash;
}
#endif
