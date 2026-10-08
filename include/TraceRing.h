#ifndef TRACE_RING_H
#define TRACE_RING_H
#include <stddef.h>
#include <stdint.h>
template <typename T, size_t Capacity>
class TraceRing {
public:
  static_assert(Capacity > 0, "Trace ring must have capacity");
  void clear() { head_=0; tail_=0; count_=0; dropped_=0; }
  void push(const T& item) {
    if (count_ == Capacity) {
      tail_=(tail_+1)%Capacity;
      ++dropped_;
    } else {
      ++count_;
    }
    items_[head_]=item;
    head_=(head_+1)%Capacity;
  }
  bool pop(T& item) {
    if (!count_) return false;
    item=items_[tail_];
    tail_=(tail_+1)%Capacity;
    --count_;
    return true;
  }
  size_t size() const { return count_; }
  uint32_t dropped() const { return dropped_; }
private:
  T items_[Capacity]{};
  size_t head_=0, tail_=0, count_=0;
  uint32_t dropped_=0;
};
#endif
