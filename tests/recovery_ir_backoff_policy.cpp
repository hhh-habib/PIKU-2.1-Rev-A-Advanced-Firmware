#include <cassert>
#include "RecoveryIrBackoffPolicy.h"

using Action = RecoveryIrBackoffPolicy::Action;

int main() {
  static_assert(RecoveryIrBackoffPolicy::PULSE_MS == 200, "pulse duration");
  static_assert(RecoveryIrBackoffPolicy::MAX_PULSES == 5, "reverse bound");
  RecoveryIrBackoffPolicy p;
  assert(p.start(1000) == Action::Backward);
  p.pulseCommanded(1005);
  assert(p.active() && p.eventPulses() == 1 && p.missionPulses() == 1);
  assert(p.update(1204, true) == Action::None);
  assert(p.update(1205, true) == Action::Stop);
  assert(p.update(1206, true) == Action::Backward);
  p.pulseCommanded(1210);
  assert(p.eventPulses() == 2);
  assert(p.update(1300, false) == Action::FrontCheck);
  assert(!p.active() && p.eventPulses() == 0);
  assert(p.update(1600, true) == Action::None); // no extra pulse after clear

  // A new IR event in the same mission cannot exceed five reverse pulses.
  assert(p.start(2000) == Action::Backward);
  assert(p.update(2200, true) == Action::Stop);
  assert(p.update(2201, true) == Action::Backward);
  assert(p.update(2401, true) == Action::Stop);
  assert(p.update(2402, true) == Action::Backward);
  assert(p.missionPulses() == 5);
  assert(p.update(2602, true) == Action::Stop);
  assert(p.update(2603, true) == Action::Failed);
  assert(!p.active());
  assert(p.start(3000) == Action::Failed); // no sixth pulse in this mission

  // A close sonar veto forces one bounded pulse even when IR is clear.
  p.resetMission();
  assert(p.start(4000, true) == Action::Backward);
  assert(p.update(4100, false) == Action::None);
  assert(p.update(4199, false) == Action::None);
  assert(p.update(4200, false) == Action::Stop);
  assert(p.update(4201, false) == Action::FrontCheck);
  assert(!p.active());

  // IR can clear before the first 200 ms: stop immediately and recheck front.
  p.resetMission();
  assert(p.start(5000) == Action::Backward);
  assert(p.update(5001, false) == Action::FrontCheck);
  assert(!p.active());
  p.resetMission(); // STOP, Manual, and hazard all cancel the policy.
  assert(p.missionPulses() == 0);
  assert(p.update(7000, true) == Action::None);

  // A persistent obstruction is bounded at five individually stopped pulses.
  assert(p.start(8000) == Action::Backward);
  uint32_t pulseStart = 8000;
  for (int i = 1; i <= 5; ++i) {
    assert(p.update(pulseStart + 200, true) == Action::Stop);
    assert(p.update(pulseStart + 201, true) ==
           (i < 5 ? Action::Backward : Action::Failed));
    pulseStart += 201;
  }
  assert(p.missionPulses() == 5);
  assert(p.update(10000, true) == Action::None);
}
