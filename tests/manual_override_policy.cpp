#include <cassert>
#include "ManualOverridePolicy.h"
int main() {
  ManualOverridePolicy p;
  assert(!p.enable(false, true, false, true, true));
  assert(!p.enable(true, false, false, true, true));
  assert(!p.enable(true, true, true, true, true));
  assert(!p.enable(true, true, false, false, true));
  assert(!p.enable(true, true, false, true, false));
  assert(p.enable(true, true, false, true, true));
  assert(p.exemptsFlameGas(true, true));
  // Direction release and 750 ms movement dead-man stop motion outside this
  // permission class. Its state must survive both events.
  p.movementStopped(); // ordinary button release
  assert(p.enabled());
  p.tick(750, true, true, false, true);
  p.movementStopped(); // movement dead-man
  assert(p.enabled());
  p.tick(2500, true, true, false, true);
  assert(p.enabled());
  p.tick(3000, true, false, false, true);
  p.tick(4999, true, false, false, true);
  assert(p.enabled());
  p.tick(5000, true, false, false, true);
  assert(!p.enabled());
  assert(p.enable(true, true, false, true, true));
  p.tick(5100, true, true, false, false); // actual session timeout
  assert(p.enabled());
  assert(!p.exemptsFlameGas(true, false)); // permission never authorizes dead-link motion
  assert(p.enable(true, true, false, true, true));
  p.tick(5200, true, true, true, true); // temperature/fault
  assert(!p.enabled());
  assert(p.enable(true, true, false, true, true));
  p.tick(5300, false, true, false, true); // AUTO/IDLE
  assert(!p.enabled());
  assert(p.enable(true, true, false, true, true));
  p.revoke(); // explicit Disable Override
  assert(!p.enabled());
}
