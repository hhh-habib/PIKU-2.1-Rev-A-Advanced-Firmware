#ifndef V2_ESCAPE_DECISION_H
#define V2_ESCAPE_DECISION_H
#include <stdint.h>

// Pure scan decision used by NavigationManager and native integration tests.
// Distances are finite fresh aggregate scan values; no-echo is invalid here.
enum class V2ScanAction { TurnLeft, TurnRight, ForwardTrial, Recovered, AlternateScan, Hold };
struct V2ScanDecisionInput {
  float leftCm=0, frontCm=0, rightCm=0;
  bool leftValid=false, frontValid=false, rightValid=false, frontStable=false;
  bool irLeft=false, irRight=false, imuReady=false;
  bool committed=false, committedLeft=false, previousFailedLeft=false;
  bool afterTurn=false, afterTrial=false, alternatePending=false, alternateTried=false;
  uint8_t sideTurns=0, totalTurns=0;
  float sideMinimumCm=30, frontMinimumCm=50, tieToleranceCm=1;
  uint8_t maxSideTurns=3, maxTotalTurns=6;
};
inline bool v2ControlLinkLost(bool requireTransport, bool transportAvailable,
                              bool requirePoll, bool pollAlive) {
  return (requireTransport && !transportAvailable) ||
         (requirePoll && !pollAlive);
}
inline V2ScanAction decideV2Scan(const V2ScanDecisionInput& in) {
  if (!in.leftValid || !in.frontValid || !in.rightValid || !in.imuReady)
    return V2ScanAction::Hold;
  const bool leftSafe=!in.irLeft && in.leftCm > in.sideMinimumCm;
  const bool rightSafe=!in.irRight && in.rightCm > in.sideMinimumCm;
  const bool trialClear=in.frontStable && !in.irLeft && !in.irRight &&
      in.frontCm >= in.frontMinimumCm && leftSafe && rightSafe;
  if (in.afterTrial && trialClear) return V2ScanAction::Recovered;
  if (in.afterTurn && !in.afterTrial && trialClear) return V2ScanAction::ForwardTrial;
  if (in.totalTurns >= in.maxTotalTurns) return V2ScanAction::Hold;
  if (in.alternatePending) {
    if (!in.committed || in.alternateTried) return V2ScanAction::Hold;
    if (in.committedLeft) return rightSafe ? V2ScanAction::TurnRight : V2ScanAction::Hold;
    return leftSafe ? V2ScanAction::TurnLeft : V2ScanAction::Hold;
  }
  if (in.committed) {
    if (in.sideTurns < in.maxSideTurns &&
        (in.committedLeft ? leftSafe : rightSafe))
      return in.committedLeft ? V2ScanAction::TurnLeft : V2ScanAction::TurnRight;
    return in.alternateTried ? V2ScanAction::Hold : V2ScanAction::AlternateScan;
  }
  if (!leftSafe && !rightSafe) return V2ScanAction::Hold;
  if (!leftSafe) return V2ScanAction::TurnRight;
  if (!rightSafe) return V2ScanAction::TurnLeft;
  if (in.leftCm > in.rightCm + in.tieToleranceCm) return V2ScanAction::TurnLeft;
  if (in.rightCm > in.leftCm + in.tieToleranceCm) return V2ScanAction::TurnRight;
  // The last normal avoidance turn failed; tie-break away from it once,
  // then retain the committed side across incremental scans.
  return in.previousFailedLeft ? V2ScanAction::TurnRight : V2ScanAction::TurnLeft;
}
#endif
