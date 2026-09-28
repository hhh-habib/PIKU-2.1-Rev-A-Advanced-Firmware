#include <cassert>
#include "RobotConfig.h"
#include "ManualMotionGate.h"
#include "ManualOverridePolicy.h"
#include "StuckRecoveryV2Policy.h"
#include "V2EscapeDecision.h"

static_assert(RobotConfig::ENABLE_STUCK_RECOVERY_V2, "physical-test build must enable V2");
static_assert(RobotConfig::AUTO_AVOIDANCE_TURN_TARGET_DEG == 35.0f, "normal turn changed");
static_assert(RobotConfig::AUTO_V2_PROVISIONAL_TURN_TARGET_DEG == 25.0f, "V2 increment changed");

struct RecoveryRig {
  StuckRecoveryV2Policy policy;
  bool motor=false, alternatePending=false, afterTrial=false;
  bool imu=true, irLeft=false, irRight=false, frontStable=true;
  float left=60, front=35, right=45;
  bool leftValid=true, frontValid=true, rightValid=true;

  void suspect(bool failedLeft=false) {
    policy.noteAvoidance(1000,0,true); policy.noteTurn(failedLeft);
    policy.noteAvoidance(2000,200,true); policy.noteTurn(!failedLeft);
    policy.noteAvoidance(3000,200,true); policy.noteTurn(failedLeft);
    assert(policy.state()==StuckV2State::Suspected);
    assert(policy.startEscape(3000));
  }
  V2ScanAction scan() {
    V2ScanDecisionInput x;
    x.leftCm=left; x.frontCm=front; x.rightCm=right;
    x.leftValid=leftValid; x.frontValid=frontValid; x.rightValid=rightValid;
    x.frontStable=frontStable; x.irLeft=irLeft; x.irRight=irRight; x.imuReady=imu;
    x.committed=policy.hasCommittedSide(); x.committedLeft=policy.committedLeft();
    x.previousFailedLeft=policy.previousFailedLeft();
    x.afterTurn=policy.state()==StuckV2State::EscapeVerify;
    x.afterTrial=afterTrial; x.alternatePending=alternatePending;
    x.alternateTried=policy.alternateTried();
    x.sideTurns=policy.sideTurns(); x.totalTurns=policy.totalTurns();
    x.hardCollisionCm=RobotConfig::NEAR_OBSTACLE_CM;
    x.frontMinimumCm=RobotConfig::AUTO_V2_PROVISIONAL_FRONT_CM;
    x.maxSideTurns=RobotConfig::AUTO_V2_MAX_TURNS_PER_SIDE;
    x.maxTotalTurns=RobotConfig::AUTO_V2_MAX_TOTAL_TURNS;
    V2ScanAction action=decideV2Scan(x);
    afterTrial=false;
    if (action==V2ScanAction::TurnLeft || action==V2ScanAction::TurnRight) {
      const bool leftTurn=action==V2ScanAction::TurnLeft;
      if (!policy.hasCommittedSide() || alternatePending) policy.commitSide(leftTurn);
      else policy.continueTurn();
      alternatePending=false;
      assert(policy.startTurn(x.maxSideTurns,x.maxTotalTurns));
      motor=true;
    } else if (action==V2ScanAction::AlternateScan) {
      policy.prepareAlternateScan(); alternatePending=true; motor=false;
    } else if (action==V2ScanAction::ForwardTrial) {
      motor=true;
    } else if (action==V2ScanAction::Recovered) {
      policy.recovered(); motor=false;
    } else {
      policy.hold(); motor=false;
    }
    return action;
  }
  void turnDone() { assert(motor); motor=false; policy.turnVerified(); }
  void trialDone() { assert(motor); motor=false; afterTrial=true; }
  void interrupt() { motor=false; policy.abort(); }
};

int main() {
  // Normal AUTO remains a separate flag-OFF path; a single scan cannot trigger V2.
  StuckRecoveryV2Policy normal;
  normal.noteAvoidance(1000,0,true); normal.noteTurn(true);
  assert(normal.state()==StuckV2State::Normal);
  normal.noteSustainedClear(5000); assert(normal.cycles()==0);
  V2ScanDecisionInput equal;
  equal.leftCm=25; equal.frontCm=30; equal.rightCm=35;
  equal.leftValid=equal.frontValid=equal.rightValid=true; equal.imuReady=true;
  assert(decideV2Scan(equal)==V2ScanAction::TurnRight); // geometry alone is not stuck evidence

  RecoveryRig boxedLeft;
  boxedLeft.suspect(false);
  boxedLeft.left=14; boxedLeft.front=9; boxedLeft.right=11;
  assert(boxedLeft.scan()==V2ScanAction::TurnLeft);
  boxedLeft.turnDone(); boxedLeft.left=13; boxedLeft.front=12; boxedLeft.right=14;
  assert(boxedLeft.scan()==V2ScanAction::TurnLeft); // committed side survives new geometry
  boxedLeft.turnDone(); boxedLeft.front=55; boxedLeft.left=12; boxedLeft.right=10;
  assert(boxedLeft.scan()==V2ScanAction::ForwardTrial);
  boxedLeft.trialDone();
  assert(boxedLeft.scan()==V2ScanAction::Recovered);

  RecoveryRig boxedRight;
  boxedRight.suspect(false);
  boxedRight.left=8; boxedRight.front=7; boxedRight.right=13;
  assert(boxedRight.scan()==V2ScanAction::TurnRight);

  RecoveryRig hardVeto;
  hardVeto.suspect(false);
  hardVeto.left=7; hardVeto.front=9; hardVeto.right=11;
  assert(hardVeto.scan()==V2ScanAction::TurnRight);
  RecoveryRig bothVeto;
  bothVeto.suspect(false);
  bothVeto.left=7; bothVeto.front=9; bothVeto.right=7;
  assert(bothVeto.scan()==V2ScanAction::Hold);
  assert(bothVeto.policy.state()==StuckV2State::TrappedHold && !bothVeto.motor);
  bothVeto.policy.hold(1000);
  assert(bothVeto.policy.retryReady(3500,RobotConfig::AUTO_V2_RETRY_COOLDOWN_MS,true));
  assert(bothVeto.policy.beginRetryRound(3500));
  bothVeto.left=14; bothVeto.right=11;
  assert(bothVeto.scan()==V2ScanAction::TurnLeft);

  RecoveryRig left;
  left.suspect(false);
  assert(left.scan()==V2ScanAction::TurnLeft);
  assert(left.policy.committedLeft() && left.policy.totalTurns()==1);
  left.turnDone();
  left.front=38; assert(left.scan()==V2ScanAction::TurnLeft); // first 25-degree increment blocked
  assert(left.policy.sideTurns()==2);
  left.turnDone();
  left.front=55; assert(left.scan()==V2ScanAction::ForwardTrial); // second increment clears
  left.trialDone();
  assert(left.scan()==V2ScanAction::Recovered);
  assert(left.policy.state()==StuckV2State::Normal && !left.motor);

  RecoveryRig right;
  right.suspect(true); right.left=45; right.right=60;
  assert(right.scan()==V2ScanAction::TurnRight);
  right.turnDone(); right.front=60;
  assert(right.scan()==V2ScanAction::ForwardTrial);
  right.trialDone(); right.front=48;
  assert(right.scan()==V2ScanAction::TurnRight); // trial did not sustain clearance
  right.turnDone(); right.front=55;
  assert(right.scan()==V2ScanAction::ForwardTrial);
  right.trialDone(); assert(right.scan()==V2ScanAction::Recovered);

  RecoveryRig exhausted;
  exhausted.suspect(false);
  for (int i=0;i<3;++i) {
    assert(exhausted.scan()==V2ScanAction::TurnLeft);
    exhausted.turnDone();
  }
  assert(exhausted.scan()==V2ScanAction::AlternateScan);
  assert(exhausted.alternatePending && !exhausted.motor);
  exhausted.left=40; exhausted.right=65;
  assert(exhausted.scan()==V2ScanAction::TurnRight);
  assert(exhausted.policy.alternateTried());
  for (int i=0;i<3;++i) {
    exhausted.turnDone();
    if (i<2) assert(exhausted.scan()==V2ScanAction::TurnRight);
  }
  assert(exhausted.scan()==V2ScanAction::Hold);
  assert(exhausted.policy.state()==StuckV2State::TrappedHold && !exhausted.motor);
  exhausted.policy.noteAvoidance(50000,100,true);
  assert(exhausted.policy.state()==StuckV2State::TrappedHold);
  exhausted.policy.hold(10000);
  assert(!exhausted.policy.retryReady(12499,RobotConfig::AUTO_V2_RETRY_COOLDOWN_MS,true));
  assert(!exhausted.policy.retryReady(12500,RobotConfig::AUTO_V2_RETRY_COOLDOWN_MS,false));
  assert(exhausted.policy.retryReady(12500,RobotConfig::AUTO_V2_RETRY_COOLDOWN_MS,true));
  assert(exhausted.policy.beginRetryRound(12500));
  assert(exhausted.policy.state()==StuckV2State::EscapeScan);
  assert(exhausted.policy.totalTurns()==0 && !exhausted.policy.hasCommittedSide());
  exhausted.front=35; exhausted.left=38; exhausted.right=70;
  assert(exhausted.scan()==V2ScanAction::TurnRight); // changed environment
  exhausted.turnDone(); exhausted.front=55;
  assert(exhausted.scan()==V2ScanAction::ForwardTrial);
  exhausted.trialDone(); assert(exhausted.scan()==V2ScanAction::Recovered);

  RecoveryRig unsafe;
  unsafe.suspect(false); unsafe.irLeft=true;
  assert(unsafe.scan()==V2ScanAction::TurnRight); // directional IR veto
  RecoveryRig noPath;
  noPath.suspect(false); noPath.irLeft=noPath.irRight=true;
  assert(noPath.scan()==V2ScanAction::Hold);
  RecoveryRig badImu;
  badImu.suspect(false); badImu.imu=false;
  assert(badImu.scan()==V2ScanAction::Hold);
  badImu.policy.hold(2000);
  assert(!badImu.policy.retryReady(5000,RobotConfig::AUTO_V2_RETRY_COOLDOWN_MS,false));
  badImu.imu=true;
  assert(badImu.policy.retryReady(5000,RobotConfig::AUTO_V2_RETRY_COOLDOWN_MS,true));
  assert(badImu.policy.beginRetryRound(5000));
  assert(badImu.scan()==V2ScanAction::TurnLeft);
  RecoveryRig noEcho;
  noEcho.suspect(false); noEcho.frontValid=false;
  assert(noEcho.scan()==V2ScanAction::Hold);
  RecoveryRig noEchoVerify;
  noEchoVerify.suspect(false); assert(noEchoVerify.scan()==V2ScanAction::TurnLeft);
  noEchoVerify.turnDone(); noEchoVerify.frontValid=false;
  assert(noEchoVerify.scan()==V2ScanAction::Hold);
  RecoveryRig symmetricTrap;
  symmetricTrap.suspect(false);
  symmetricTrap.left=symmetricTrap.front=symmetricTrap.right=30;
  assert(symmetricTrap.scan()==V2ScanAction::TurnRight); // tie uses failed-turn history
  RecoveryRig unstable;
  unstable.suspect(false); assert(unstable.scan()==V2ScanAction::TurnLeft);
  unstable.turnDone(); unstable.front=60; unstable.frontStable=false;
  assert(unstable.scan()==V2ScanAction::TurnLeft); // one unstable front does not authorize trial

  // STOP, flame/gas, Manual takeover, link loss, stale IMU all stop a running
  // turn in NavigationManager before the next state action. Policy abort/hold
  // is exercised here without pretending to run the ESP32 peripherals.
  for (int cause=0;cause<5;++cause) {
    RecoveryRig interrupted;
    interrupted.suspect(false);
    assert(interrupted.scan()==V2ScanAction::TurnLeft);
    if (cause==4) { interrupted.motor=false; interrupted.policy.hold(); }
    else interrupted.interrupt();
    assert(!interrupted.motor);
    assert(interrupted.policy.state()==(cause==4 ? StuckV2State::TrappedHold
                                                 : StuckV2State::Normal));
  }
  assert(!v2ControlLinkLost(false,false,false,false)); // standalone AUTO
  assert(!v2ControlLinkLost(true,true,true,true));
  assert(v2ControlLinkLost(true,false,false,false)); // prior transport vanished
  assert(v2ControlLinkLost(false,false,true,false)); // prior poll timed out
  StuckRecoveryV2Policy stopDuringCooldown;
  stopDuringCooldown.hold(1000);
  stopDuringCooldown.abort();
  assert(!stopDuringCooldown.retryReady(4000,RobotConfig::AUTO_V2_RETRY_COOLDOWN_MS,true));
  RecoveryRig timeout;
  timeout.suspect(false);
  assert(timeout.policy.escapeTimedOut(3000+RobotConfig::AUTO_V2_TOTAL_TIMEOUT_MS,
                                      RobotConfig::AUTO_V2_TOTAL_TIMEOUT_MS));
  timeout.policy.hold(); assert(timeout.policy.state()==StuckV2State::TrappedHold);

  ManualGateInput m;
  m.manualMode=true; m.direction=ManualDirection::Left;
  assert(manualMotionGate(m)==ManualInhibit::None);
  m.leftTurnObstacle=true; assert(manualMotionGate(m)==ManualInhibit::TurnObstacle);
  m.leftTurnObstacle=false; m.direction=ManualDirection::Right;
  assert(manualMotionGate(m)==ManualInhibit::None);
  m.rightTurnObstacle=true; assert(manualMotionGate(m)==ManualInhibit::TurnObstacle);
  m.rightTurnObstacle=false; m.direction=ManualDirection::Forward;
  m.forwardObstacle=true; assert(manualMotionGate(m)==ManualInhibit::ForwardObstacle);
  m.forwardObstacle=false; m.flameGasCritical=true; m.overrideEnabled=true; m.sessionAlive=true;
  m.direction=ManualDirection::Left; assert(manualMotionGate(m)==ManualInhibit::None);
  m.temperatureCritical=true; assert(manualMotionGate(m)==ManualInhibit::TemperatureCritical);
  m.temperatureCritical=false; m.leftTurnObstacle=true;
  assert(manualMotionGate(m)==ManualInhibit::TurnObstacle);
  m.leftTurnObstacle=false; m.direction=ManualDirection::Backward;
  assert(manualMotionGate(m)==ManualInhibit::None); // operator-supervised rear
  ManualOverridePolicy permission;
  assert(permission.enable(true,true,false,true,true));
  m.staleCommand=true; m.direction=ManualDirection::Left;
  assert(manualMotionGate(m)==ManualInhibit::StaleCommand); // movement dead-man
  permission.movementStopped();
  assert(permission.enabled()); // turning stops; override remains until link loss
  permission.tick(1000,true,true,false,false);
  assert(!permission.enabled());
  m.direction=ManualDirection::Stop;
  assert(manualMotionGate(m)==ManualInhibit::OperatorStop);
  return 0;
}
