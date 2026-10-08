#include "StuckRecoveryV2Policy.h"
#include <cassert>
// Reference elapsed mission time; retry starts a new mission, not a direction ledger.
int main(){StuckRecoveryV2Policy p;p.noteBoxedIn();assert(p.startEscape(100));assert(!p.escapeTimedOut(30099,30000));assert(p.escapeTimedOut(30100,30000));p.hold(30100);assert(!p.retryReady(32599,2500));assert(p.retryReady(32600,2500));assert(p.beginRetryRound(32600));assert(!p.escapeTimedOut(62599,30000));assert(p.escapeTimedOut(62600,30000));p.abort();assert(!p.episodeActive());p.noteBoxedIn();assert(p.startEscape(UINT32_MAX-10000));assert(!p.escapeTimedOut(19998,30000));assert(p.escapeTimedOut(19999,30000));}
