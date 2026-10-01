#include <cassert>
#include "StuckRecoveryV2Policy.h"

int main() {
  StuckRecoveryV2Policy p;
  p.noteAvoidance(1000,0,true); p.noteTurn(true);
  assert(p.state()==StuckV2State::Normal);
  p.noteAvoidance(2000,200,true); p.noteTurn(false);
  p.noteAvoidance(3000,200,true); p.noteTurn(true);
  assert(p.state()==StuckV2State::Suspected);
  assert(p.startEscape(3000));
  p.commitSide(true);
  p.commitSide(false); // direction cannot change within this mission
  assert(p.committedLeft());
  for (int i=0;i<7;++i) {
    assert(p.startTurn(7));
    p.turnVerified();
    if (i<6) p.continueTurn();
  }
  assert(!p.startTurn(7));
  p.hold(10000);
  assert(!p.retryReady(12499,2500));
  assert(p.retryReady(12500,2500));
  assert(p.beginRetryRound(12500));
  assert(!p.hasCommittedSide() && p.sideTurns()==0);
  p.commitSide(false); // new mission may select a new direction
  assert(!p.committedLeft());
  p.abort(); // STOP, Manual takeover, or hazard
  assert(p.state()==StuckV2State::Normal);
  p.rearm();
  p.noteBoxedIn();
  assert(p.state()==StuckV2State::Suspected);
  assert(p.startEscape(20000));
  p.recovered();
  assert(p.state()==StuckV2State::Normal && p.attempts()==0);
  p.rearm();
  assert(!p.noteInvalidScan(1));
  assert(!p.noteInvalidScan(2000));
  assert(p.noteInvalidScan(3000));
}
