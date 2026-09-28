#include <cassert>
#include "StuckRecoveryV2Policy.h"
int main() {
  StuckRecoveryV2Policy p;
  // Similar L/F/R values alone never enter this history detector.
  assert(p.state()==StuckV2State::Normal);
  p.noteAvoidance(1000,0,true); p.noteTurn(true); // ordinary single obstacle
  assert(p.state()==StuckV2State::Normal);
  p.noteSustainedClear(5000); // narrow corridor with sustained commanded clearance
  assert(p.cycles()==0);
  p.noteAvoidance(7000,0,true); p.noteTurn(true);
  p.noteAvoidance(9000,300,true); p.noteTurn(false);
  p.noteAvoidance(11000,250,true); p.noteTurn(true); // oscillation
  assert(p.state()==StuckV2State::Suspected);
  assert(p.startEscape(11000));
  assert(p.state()==StuckV2State::EscapeScan);
  assert(!p.escapeTimedOut(25999));
  assert(p.escapeTimedOut(26000));
  p.hold(); assert(p.state()==StuckV2State::TrappedHold);
  p.noteAvoidance(30000,100,true); // hold is persistent
  assert(p.state()==StuckV2State::TrappedHold);
  p.rearm(); assert(p.state()==StuckV2State::Normal);
  p.noteAvoidance(30010,0,true); p.noteBlockedScan();
  p.noteAvoidance(31010,0,true); p.noteBlockedScan();
  p.noteAvoidance(32010,0,true); p.noteBlockedScan();
  assert(p.state()==StuckV2State::Suspected); // U-shaped no-safe scans
  p.rearm();
  assert(!p.noteInvalidScan(1));
  assert(!p.noteInvalidScan(2000));
  assert(p.noteInvalidScan(3000)); // sensor fault needs assistance, not a turn
  p.rearm();
  assert(!p.noteInvalidScan(1));
  assert(!p.noteInvalidScan(20000)); // old invalid evidence expires
  p.rearm();
  // Faulty sonar and isolated retries do not trigger.
  p.noteAvoidance(32000,100,false);
  assert(p.cycles()==0);
  p.noteAvoidance(33000,0,true); p.noteTurn(false);
  p.noteAvoidance(34000,200,true); p.noteTurn(false);
  p.noteAvoidance(35000,250,true); p.noteTurn(false);
  assert(p.state()==StuckV2State::Suspected); // repeated short failed steps
  assert(p.startEscape(35000));
  p.scanVerified(); assert(p.state()==StuckV2State::EscapeTurn);
  p.turnVerified(); assert(p.state()==StuckV2State::EscapeVerify);
  p.abort(); // STOP, local hazard or manual takeover
  assert(p.state()==StuckV2State::Normal && p.attempts()==1);
  for (int i=0;i<2;++i) {
    unsigned t=40000+i*4000;
    p.noteAvoidance(t,0,true); p.noteTurn(true);
    p.noteAvoidance(t+1000,200,true); p.noteTurn(false);
    p.noteAvoidance(t+2000,200,true); p.noteTurn(true);
    assert(p.startEscape(t+2000));
    p.abort();
  }
  assert(p.attempts()==3);
  p.noteAvoidance(51000,0,true); p.noteTurn(true);
  p.noteAvoidance(52000,200,true); p.noteTurn(false);
  p.noteAvoidance(53000,200,true); p.noteTurn(true);
  assert(!p.startEscape(53000));
  assert(p.state()==StuckV2State::TrappedHold);
}
