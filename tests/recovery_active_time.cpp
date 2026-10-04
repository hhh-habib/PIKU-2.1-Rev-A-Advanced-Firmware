#include "StuckRecoveryV2Policy.h"
#include <cassert>
int main(){
 StuckRecoveryV2Policy p;p.noteBoxedIn();assert(p.startEscape(100));
 p.pause(12100);assert(p.activeRecoveryMs(32100)==12000);
 p.resume(32100);p.pause(37100);assert(p.activeRecoveryMs(60000)==17000);
 p.resume(60000);assert(!p.escapeTimedOut(72999,30000));assert(p.escapeTimedOut(73000,30000));
 p.pause(73000);p.resume(80000);assert(p.escapeTimedOut(80000,30000));
 p.rearm();assert(p.activeRecoveryMs(80000)==0&&!p.episodeActive());
 p.noteBoxedIn();assert(p.startEscape(90000));p.pause(119999);
 p.resume(150000);assert(!p.escapeTimedOut(150000,30000));assert(p.escapeTimedOut(150001,30000));
 p.abort();assert(p.activeRecoveryMs(150001)==0);
 p.noteBoxedIn();assert(p.startEscape(UINT32_MAX-999));p.pause(1000);assert(p.activeRecoveryMs(5000)==2000);
 p.resume(5000);assert(p.activeRecoveryMs(6000)==3000);
 p.handoffCandidate(6000);assert(p.activeRecoveryMs(90000)==3000);
 p.pause(90000);p.resume(90000);assert(p.activeRecoveryMs(91000)==4000);
 p.hold(91000);p.pause(92000);p.hold(93000);assert(!p.retryReady(93499,2500));assert(p.retryReady(93500,2500));
 assert(p.beginRetryRound(93500));assert(p.activeRecoveryMs(94500)==5000);
 p.handoffCandidate(94500);p.noteSustainedClear(5000);assert(!p.episodeActive()&&p.activeRecoveryMs(99000)==0);
}
