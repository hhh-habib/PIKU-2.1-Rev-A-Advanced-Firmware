#include "StuckRecoveryV2Policy.h"
#include "RecoveryIrBackoffPolicy.h"
#include <cassert>
// Per-mission turn/pulse limits reset at the original automatic retry boundary.
int main(){StuckRecoveryV2Policy p;RecoveryIrBackoffPolicy ir;p.noteBoxedIn();assert(p.startEscape(100));p.commitSide(true);for(unsigned i=0;i<7;++i){assert(p.startTurn(7));p.turnVerified();if(i<6)p.continueTurn();}assert(!p.startTurn(7));for(unsigned i=0;i<5;++i){assert(ir.start(1000+i*500,true)==RecoveryIrBackoffPolicy::Action::Backward);ir.cancelPulse();}assert(ir.start(4000,true)==RecoveryIrBackoffPolicy::Action::Failed);p.hold(4000);assert(p.beginRetryRound(6500));ir.resetMission();assert(p.sideTurns()==0&&p.attempts()==2);assert(ir.start(6500,true)==RecoveryIrBackoffPolicy::Action::Backward);p.abort();ir.resetMission();assert(!p.episodeActive()&&ir.missionPulses()==0);}
