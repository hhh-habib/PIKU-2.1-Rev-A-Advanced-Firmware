#ifndef V2_ESCAPE_DECISION_H
#define V2_ESCAPE_DECISION_H
#include <stdint.h>

// Pure recovery decisions. A clear no-echo is open; fault/stale is untrusted.
enum class RecoveryReadingState { DistanceValid, ClearNoEcho, Invalid };
struct RecoveryReading {
  RecoveryReadingState state=RecoveryReadingState::Invalid;
  float cm=0;
};
enum class V2ScanAction { TurnLeft, TurnRight, Hold };
struct V2ScanDecisionInput {
  RecoveryReading left, front, right;
  bool imuReady=false;
  bool previousFailedLeft=false;
  float hardCollisionCm=7, tieToleranceCm=1;
};
inline bool recoveryReadingTrusted(const RecoveryReading& reading) {
  return reading.state != RecoveryReadingState::Invalid;
}
inline bool recoveryPathSafe(const RecoveryReading& reading, float hardCollisionCm) {
  return reading.state == RecoveryReadingState::ClearNoEcho ||
      (reading.state == RecoveryReadingState::DistanceValid &&
       reading.cm > hardCollisionCm);
}
inline bool recoveryFrontClear(const RecoveryReading& reading, float exitCm) {
  return reading.state == RecoveryReadingState::ClearNoEcho ||
      (reading.state == RecoveryReadingState::DistanceValid &&
       reading.cm >= exitCm);
}
inline bool obviousBoxedIn(const V2ScanDecisionInput& in, float boxedSideCm) {
  return in.front.state == RecoveryReadingState::DistanceValid &&
      in.front.cm < boxedSideCm &&
      in.left.state == RecoveryReadingState::DistanceValid &&
      in.right.state == RecoveryReadingState::DistanceValid &&
      in.left.cm < boxedSideCm && in.right.cm < boxedSideCm;
}
inline V2ScanAction decideV2Scan(const V2ScanDecisionInput& in) {
  if (!recoveryReadingTrusted(in.left) || !recoveryReadingTrusted(in.front) ||
      !recoveryReadingTrusted(in.right) || !in.imuReady)
    return V2ScanAction::Hold;
  const bool leftSafe=recoveryPathSafe(in.left,in.hardCollisionCm);
  const bool rightSafe=recoveryPathSafe(in.right,in.hardCollisionCm);
  if (!leftSafe && !rightSafe) return V2ScanAction::Hold;
  if (!leftSafe) return V2ScanAction::TurnRight;
  if (!rightSafe) return V2ScanAction::TurnLeft;
  if (in.left.state == RecoveryReadingState::ClearNoEcho)
    return V2ScanAction::TurnLeft;
  if (in.right.state == RecoveryReadingState::ClearNoEcho)
    return V2ScanAction::TurnRight;
  if (in.left.cm > in.right.cm + in.tieToleranceCm) return V2ScanAction::TurnLeft;
  if (in.right.cm > in.left.cm + in.tieToleranceCm) return V2ScanAction::TurnRight;
  return in.previousFailedLeft ? V2ScanAction::TurnRight : V2ScanAction::TurnLeft;
}
#endif
