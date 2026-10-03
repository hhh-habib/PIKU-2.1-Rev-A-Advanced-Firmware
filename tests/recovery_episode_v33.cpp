#include <cassert>
#include "StuckRecoveryV2Policy.h"
#include "RecoveryIrBackoffPolicy.h"
int main() {
 StuckRecoveryV2Policy p; RecoveryIrBackoffPolicy ir;
 p.noteBoxedIn(); assert(p.startEscape(100)); p.commitSide(true);
 for (unsigned i=0;i<6;++i) {
   assert(p.startTurn(6)); // consumed before completion, including interruptions
   p.pause(200+i); assert(p.sideTurns()==i+1 && p.committedLeft());
   p.hold(1000+i); assert(p.beginRetryRound(4000+i)); p.commitSide(false);
   assert(p.committedLeft() && p.sideTurns()==i+1);
 }
 assert(!p.startTurn(6)); p.handoffCandidate();
 p.noteSustainedClear(4999); assert(p.episodeActive() && p.sideTurns()==6);
 p.pause(6000); assert(p.episodeActive() && !p.handoffPending());
 p.handoffCandidate(); p.noteSustainedClear(5000);
 assert(!p.episodeActive() && p.sideTurns()==0);
 for(unsigned i=0;i<5;++i) {
   assert(ir.start(i*500,true)==RecoveryIrBackoffPolicy::Action::Backward);
   ir.cancelPulse(); assert(ir.missionPulses()==i+1);
 }
 assert(ir.start(3000)==RecoveryIrBackoffPolicy::Action::Failed);
 ir.resetMission(); assert(ir.missionPulses()==0);
 p.noteBoxedIn(); assert(p.startEscape(7000)); p.commitSide(true);
 assert(p.startTurn(6)); p.pause(8000); assert(p.sideTurns()==1);
 p.rearm(); assert(!p.episodeActive() && p.sideTurns()==0);
}
