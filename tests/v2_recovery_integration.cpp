#include <cassert>
#include "RobotConfig.h"
#include "StuckRecoveryV2Policy.h"
#include "V2EscapeDecision.h"

static_assert(RobotConfig::AUTO_AVOIDANCE_TURN_TARGET_DEG == 35.0f,
              "normal AUTO turn changed");
static_assert(RobotConfig::RECOVERY_TURN_TARGET_DEG == 25.0f,
              "recovery increment changed");
static_assert(RobotConfig::RECOVERY_FRONT_EXIT_CM == 30.0f,
              "recovery handoff changed");
static_assert(RobotConfig::RECOVERY_HARD_VETO_CM == 7.0f,
              "hard collision veto changed");
static_assert(RobotConfig::RECOVERY_MAX_TURNS == 7,
              "recovery mission limit changed");

static RecoveryReading valid(float cm) {
  RecoveryReading r;
  r.state=RecoveryReadingState::DistanceValid; r.cm=cm; return r;
}
static RecoveryReading noEcho() {
  RecoveryReading r;
  r.state=RecoveryReadingState::ClearNoEcho; return r;
}
static RecoveryReading invalid() { return RecoveryReading(); }

static V2ScanDecisionInput scan(RecoveryReading l, RecoveryReading f,
                                RecoveryReading r) {
  V2ScanDecisionInput x;
  x.left=l; x.front=f; x.right=r; x.imuReady=true;
  x.hardCollisionCm=RobotConfig::RECOVERY_HARD_VETO_CM;
  return x;
}

int main() {
  StuckRecoveryV2Policy normal;
  normal.noteAvoidance(1000,0,true); normal.noteTurn(true);
  assert(normal.state()==StuckV2State::Normal); // ordinary AUTO stays normal

  auto boxed=scan(valid(10),valid(6),valid(15));
  assert(obviousBoxedIn(boxed,RobotConfig::AUTO_OBSTACLE_CM,
                       RobotConfig::RECOVERY_BOXED_SIDE_CM));
  assert(decideV2Scan(boxed)==V2ScanAction::TurnRight);
  boxed=scan(valid(25),valid(10),valid(15));
  assert(obviousBoxedIn(boxed,RobotConfig::AUTO_OBSTACLE_CM,
                       RobotConfig::RECOVERY_BOXED_SIDE_CM));
  assert(decideV2Scan(boxed)==V2ScanAction::TurnLeft);
  assert(!obviousBoxedIn(scan(valid(45),valid(6),valid(15)),
                          RobotConfig::AUTO_OBSTACLE_CM,
                          RobotConfig::RECOVERY_BOXED_SIDE_CM));

  auto openLeft=scan(noEcho(),valid(8),valid(18));
  assert(!obviousBoxedIn(openLeft,RobotConfig::AUTO_OBSTACLE_CM,
                        RobotConfig::RECOVERY_BOXED_SIDE_CM));
  assert(decideV2Scan(openLeft)==V2ScanAction::TurnLeft);
  assert(decideV2Scan(scan(valid(14),valid(7),valid(10)))==
         V2ScanAction::TurnLeft);
  assert(decideV2Scan(scan(valid(8),valid(6),valid(18)))==
         V2ScanAction::TurnRight);

  StuckRecoveryV2Policy mission;
  mission.noteBoxedIn();
  assert(mission.startEscape(1000));
  mission.commitSide(true);
  assert(mission.startTurn(RobotConfig::RECOVERY_MAX_TURNS));
  mission.turnVerified();
  mission.commitSide(false);
  assert(mission.committedLeft()); // later right-side evidence cannot flip it
  assert(recoveryFrontClear(valid(31),RobotConfig::RECOVERY_FRONT_EXIT_CM));
  assert(recoveryFrontClear(noEcho(),RobotConfig::RECOVERY_FRONT_EXIT_CM));
  assert(!recoveryFrontClear(valid(22),RobotConfig::RECOVERY_FRONT_EXIT_CM));
  assert(!recoveryFrontClear(invalid(),RobotConfig::RECOVERY_FRONT_EXIT_CM));
  assert(recoveryPathSafe(valid(45),RobotConfig::RECOVERY_HARD_VETO_CM));
  assert(recoveryPathSafe(valid(28),RobotConfig::RECOVERY_HARD_VETO_CM));
  assert(recoveryPathSafe(noEcho(),RobotConfig::RECOVERY_HARD_VETO_CM));
  assert(!recoveryPathSafe(valid(7),RobotConfig::RECOVERY_HARD_VETO_CM));
  assert(!recoveryPathSafe(valid(6),RobotConfig::RECOVERY_HARD_VETO_CM));
  assert(!recoveryPathSafe(invalid(),RobotConfig::RECOVERY_HARD_VETO_CM));
  mission.continueTurn();
  for (int i=1;i<RobotConfig::RECOVERY_MAX_TURNS;++i) {
    assert(mission.startTurn(RobotConfig::RECOVERY_MAX_TURNS));
    mission.turnVerified();
    if (i+1<RobotConfig::RECOVERY_MAX_TURNS) mission.continueTurn();
  }
  assert(mission.sideTurns()==RobotConfig::RECOVERY_MAX_TURNS);
  mission.hold(20000); // bounded failure, motors remain off in manager
  assert(!mission.retryReady(22499,2500));
  assert(mission.retryReady(22500,2500));
  assert(mission.beginRetryRound(22500));
  mission.commitSide(false);
  assert(!mission.committedLeft()); // new mission may choose right
  mission.abort(); // STOP / Manual / hazard cancels old direction
  assert(!mission.hasCommittedSide() && mission.state()==StuckV2State::Normal);
  mission.noteBoxedIn(); assert(mission.startEscape(30000));
  mission.commitSide(true); mission.recovered(); // no second AUTO command
  assert(mission.state()==StuckV2State::Normal);
  assert(decideV2Scan(scan(invalid(),valid(6),valid(18)))==
         V2ScanAction::Hold);
  assert(decideV2Scan(scan(valid(10),invalid(),valid(15)))==
         V2ScanAction::Hold);
  assert(decideV2Scan(scan(valid(6),valid(6),valid(7)))==
         V2ScanAction::Hold);
}
