#include <cassert>
#include "TraceChecksum.h"
int main() {
  assert(traceChecksum("hello",5)==0x4f9f2cabUL);
  assert(traceChecksum("",0)==2166136261UL);
}
