#include <cassert>
#include <cstdint>
#include "StaReconnectPolicy.h"

int main() {
  // Each check only reads clocks and state; radio work remains in the loop.
  assert(!staRetryDue(1499, 1, 1500));
  assert(staRetryDue(1501, 1, 1500));
  assert(staNextRetryInterval(1500, 8000)==3000);
  assert(staNextRetryInterval(3000, 8000)==6000);
  assert(staNextRetryInterval(6000, 8000)==8000);
  assert(staNextRetryInterval(8000, 8000)==8000);
  assert(staRetryDue(20, UINT32_MAX-1499, 1500));
  return 0;
}
