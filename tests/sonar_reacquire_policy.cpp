#include <cassert>
#include "SonarReacquirePolicy.h"
#include "RobotConfig.h"

int main() {
  static_assert(RobotConfig::SONAR_FAST_REACQUIRE_WINDOW_MS == 1000,
                "fast observation window changed");
  SonarReacquirePolicy p;
  p.begin(100, 10);
  assert(!p.observe(11, true)); // a sample during servo settle is ignored
  p.centered(11);
  assert(!p.observe(11, true)); // must be a new front sample
  assert(!p.observe(12, true)); // first healthy sample
  assert(p.consecutiveHealthy() == 1);
  assert(!p.observe(12, true)); // cannot count the same result twice
  assert(!p.observe(13, false)); // fault breaks consecutive confirmation
  assert(p.consecutiveHealthy() == 0);
  assert(!p.observe(14, true)); // trusted finite distance
  assert(p.observe(15, true)); // trusted ClearNoEcho is equally healthy
  assert(!p.windowExpired(1099, 1000));
  assert(p.windowExpired(1100, 1000));
  p.begin(2000, 15);
  p.centered(15);
  for (uint32_t seq = 16; seq < 40; ++seq)
    assert(!p.observe(seq, false)); // persistent fault never authorizes motion
  assert(p.windowExpired(3000, 1000));
  assert(!p.observe(40, true));
  assert(p.observe(41, true)); // later recovery still succeeds without reset
  p.begin(4000, 41); // STOP / Manual / hazard discard partial confirmation
  assert(p.consecutiveHealthy() == 0);
  p.centered(41);
  assert(!p.observe(42, true));
}
