#define main previous_collision_main
#include "native_state_collisions.cpp"
#undef main

void scanUntil(Rig& r, NavigationManager::AutoPhase target, float front=60) {
  for(unsigned i=0;i<100 && r.nav._phase!=target;++i) {
    if(r.nav._phase==NavigationManager::AutoPhase::ScanSampleWait) {
      r.feed(UltrasonicResultState::DistanceValid,r.nav._scanDirectionIndex==1?front:80);
      r.sensors.sample.facingFront=r.nav._scanDirectionIndex==1;
    }
    r.step(100);
  }
  assert(r.nav._phase==target);
}
void exhaust(Rig& r,unsigned over=0,bool fullLedger=false) {
  r.episode();
  r.nav._v2.commitSide(true);assert(r.nav._v2.startTurn(6));
  r.nav._recoveryIrBackoff.start(testNow);r.nav._recoveryIrBackoff.cancelPulse();
  if(fullLedger){for(unsigned i=1;i<6;++i)assert(r.nav._v2.startTurn(6));for(unsigned i=1;i<5;++i){r.nav._recoveryIrBackoff.start(testNow);r.nav._recoveryIrBackoff.cancelPulse();}}
  r.nav._phase=NavigationManager::AutoPhase::Turn;r.motors.spinLeft();
  const auto spins=r.motors.spins,backwards=r.motors.backwards;
  r.step(RobotConfig::RECOVERY_MISSION_TIMEOUT_MS+over);
  assert(r.motors.motion==MotorState::Stopped&&r.motors.spins==spins&&r.motors.backwards==backwards);
  assert(r.nav._v2.watchdogExhausted()&&r.nav._v2.sideTurns()==(fullLedger?6:1)&&r.nav._recoveryIrBackoff.missionPulses()==(fullLedger?5:1));
  assert(!r.nav._recoveryIrBackoff.active());
}
void provisional(Rig& r) {
  r.step(RobotConfig::RECOVERY_COOLDOWN_MS);
  for(unsigned i=0;i<20&&!r.nav._v2.handoffPending();++i){r.feed();r.step(RobotConfig::AUTO_FRONT_CENTER_SETTLE_MS);}
  assert(r.nav._v2.handoffPending()&&r.nav._v2.watchdogExhausted());
}

int main(int argc, char** argv) {
  assert(argc == 2); String name=argv[1]; Rig r;
  if (name=="left" || name=="right" || name=="overlay_ir" || name=="overlay_verify" || name=="overlay_trial" || name=="overlay_hold" || name=="overlay_exhausted") {
    if(name=="overlay_exhausted")exhaust(r);
    else if(name.substr(0,7)=="overlay") {
      r.episode();
      if(name=="overlay_verify"){r.nav._v2.commitSide(true);assert(r.nav._v2.startTurn(6));r.nav._v2.turnVerified();}
      if(name=="overlay_trial")r.nav._v2.handoffCandidate(testNow);
      if(name=="overlay_hold")r.nav._v2.hold(testNow);
    }
    r.environmental();
    r.safety.left=name!="right"; r.safety.right=name=="right";
    // Keep IR active through the entire scan, decision and next dispatch.
    for (unsigned i=0;i<100;++i) {
      if(r.nav._phase==NavigationManager::AutoPhase::ScanSampleWait) {
        r.feed();r.sensors.sample.facingFront=r.nav._scanDirectionIndex==1;
      }
      r.step(100);
      assert(r.nav.environmentalRecoveryInProgress());
      assert(!r.nav.consumeEnvironmentalRecoveryCompleted());
      assert(r.motors.motion==MotorState::Stopped && r.motors.backwards==0);
    }
    r.safety.left=r.safety.right=false;r.feed();r.step();r.freshEnvironmentalScan();
  } else if (name=="post_ir") {
    r.environmental();
    r.nav._phase=NavigationManager::AutoPhase::PostTurnSettle;
    r.safety.left=true;r.feed();r.step(RobotConfig::AUTO_POST_TURN_SETTLE_MS);
    assert(r.nav.environmentalRecoveryInProgress());
    assert(!r.nav.consumeEnvironmentalRecoveryCompleted());
    r.step();assert(r.motors.backwards==0 && r.motors.motion==MotorState::Stopped);
  } else if (name=="watchdog") {
    r.episode();r.step(RobotConfig::RECOVERY_MISSION_TIMEOUT_MS);
    assert(r.motors.motion==MotorState::Stopped);
    const auto spent=r.nav._v2.activeRecoveryMs(testNow);
    r.step(RobotConfig::RECOVERY_COOLDOWN_MS);
    for(unsigned i=0;i<20 && !r.nav._v2.handoffPending();++i) {
      r.feed();r.step(RobotConfig::AUTO_FRONT_CENTER_SETTLE_MS);
    }
    assert(r.nav._v2.handoffPending());
    assert(r.nav._v2.activeRecoveryMs(testNow)==spent);
    assert(r.nav._v2.episodeActive());
  } else if(name=="decision_ir"||name=="turn_ir"||name=="both_ir"||name=="post_front") {
    r.environmental();scanUntil(r,NavigationManager::AutoPhase::RecenterSettle,10);
    if(name=="decision_ir"||name=="both_ir") {
      r.safety.left=true;r.safety.right=name=="both_ir";
      r.step(RobotConfig::AUTO_SCAN_RECENTER_SETTLE_MS);
    } else {
      r.step(RobotConfig::AUTO_SCAN_RECENTER_SETTLE_MS);
      assert(r.nav._phase==NavigationManager::AutoPhase::Turn);
      if(name=="turn_ir") {r.safety.right=true;r.step();}
      else {
        // Reach the real completion/settle bodies after an actual environmental turn.
        r.nav.completeSpin("TEST_IMU_TARGET",testNow);
        r.step(RobotConfig::AUTO_POST_TURN_SETTLE_MS);
        assert(r.nav.environmentalRecoveryInProgress());
        assert(r.nav._phase==NavigationManager::AutoPhase::FrontSettle);
        r.step(RobotConfig::AUTO_FRONT_CENTER_SETTLE_MS);r.feed();r.step();
        assert(!r.nav.environmentalRecoveryInProgress());
        return 0;
      }
    }
    assert(r.nav.environmentalRecoveryInProgress()&&!r.nav.consumeEnvironmentalRecoveryCompleted());
    assert(r.motors.motion==MotorState::Stopped&&r.motors.backwards==0);
    r.safety.left=r.safety.right=false;r.feed();r.step();r.freshEnvironmentalScan();
  } else if(name=="watchdog_exact"||name=="watchdog_over"||name=="watchdog_proof") {
    exhaust(r,name=="watchdog_over"?123:0);const auto spent=r.nav._v2.activeRecoveryMs(testNow);
    r.step(RobotConfig::RECOVERY_COOLDOWN_MS-1);assert(r.nav._v2.activeRecoveryMs(testNow)==spent);
    provisional(r);assert(r.nav._v2.sideTurns()==1&&r.nav._recoveryIrBackoff.missionPulses()==1);
    if(name=="watchdog_proof") {
      unsigned forwardEvidence=0;
      for(unsigned i=0;i<500&&r.nav._v2.episodeActive();++i) {
        if(r.nav._phase==NavigationManager::AutoPhase::Forward)forwardEvidence+=100;
        r.feed();r.step(100);
        if(r.nav._v2.episodeActive())assert(r.nav._v2.activeRecoveryMs(testNow)==spent&&r.nav._v2.sideTurns()==1&&r.nav._recoveryIrBackoff.missionPulses()==1);
      }
      assert(!r.nav._v2.episodeActive()&&forwardEvidence>=5000);
      assert(r.nav._v2.sideTurns()==0&&r.nav._recoveryIrBackoff.missionPulses()==0);
    }
  } else if(name=="watchdog_ir"||name=="watchdog_sonar"||name=="watchdog_hazard"||name=="watchdog_block") {
    exhaust(r);const auto spent=r.nav._v2.activeRecoveryMs(testNow);provisional(r);
    for(unsigned i=0;i<20&&r.motors.motion!=MotorState::Forward;++i){r.feed();r.step(100);}
    assert(r.motors.motion==MotorState::Forward);
    if(name=="watchdog_ir")r.safety.left=true;
    else if(name=="watchdog_sonar")r.feed(UltrasonicResultState::SensorFault);
    else if(name=="watchdog_hazard")r.safety.hazard=true;
    else r.feed(UltrasonicResultState::DistanceValid,10);
    r.step();assert(r.motors.motion==MotorState::Stopped);
    assert(!r.nav._v2.handoffPending()&&r.nav._v2.watchdogExhausted());
    assert(r.nav._v2.activeRecoveryMs(testNow)==spent&&r.nav._v2.sideTurns()==1&&r.nav._recoveryIrBackoff.missionPulses()==1);
    assert(r.motors.backwards==0);
  } else if(name=="watchdog_cycles"||name=="watchdog_full_ledger") {
    exhaust(r,0,name=="watchdog_full_ledger");
    for(unsigned cycle=0;cycle<3;++cycle) {
      const auto expired=testNow; const auto spent=r.nav._v2.activeRecoveryMs(testNow);
      r.feed(UltrasonicResultState::DistanceValid,15);r.step(RobotConfig::RECOVERY_COOLDOWN_MS);
      for(unsigned i=0;i<150&&r.nav._v2.watchdogExhausted();++i) {
        if(r.nav._phase==NavigationManager::AutoPhase::ScanSampleWait){r.feed(UltrasonicResultState::DistanceValid,r.nav._scanDirectionIndex==1?15.0f:80.0f);r.sensors.sample.facingFront=r.nav._scanDirectionIndex==1;}
        else r.feed(UltrasonicResultState::DistanceValid,15);
        r.step(100);
        if(r.nav._v2.watchdogExhausted())assert(r.motors.motion==MotorState::Stopped&&r.nav._v2.activeRecoveryMs(testNow)==spent);
      }
      assert(!r.nav._v2.watchdogExhausted()&&r.nav._v2.episodeStartedMs()>expired);
      assert(r.nav._leftScan.ultrasonic.sampleMs>expired&&r.nav._frontScan.ultrasonic.sampleMs>expired&&r.nav._rightScan.ultrasonic.sampleMs>expired);
      assert(r.nav._v2.sideTurns()==1&&r.nav._recoveryIrBackoff.missionPulses()==0);
      assert(r.nav._v2.activeRecoveryMs(testNow)==0);
      r.step(RobotConfig::RECOVERY_MISSION_TIMEOUT_MS);
      assert(r.nav._v2.watchdogExhausted()&&r.motors.motion==MotorState::Stopped);
    }
  } else if(name=="env_ir_sonar") {
    r.environmental();r.safety.left=true;r.step();r.feed(UltrasonicResultState::SensorFault);r.step();
    assert(r.nav.environmentalRecoveryInProgress()&&r.motors.motion==MotorState::Stopped);
    r.restoreSonar();r.safety.left=false;r.feed();r.step();r.freshEnvironmentalScan();assert(r.motors.backwards==0);
  } else if(name=="env_invalid_completion") {
    r.environmental();scanUntil(r,NavigationManager::AutoPhase::RecenterSettle);
    r.nav._frontScan.ultrasonic.sampleMs=testNow-RobotConfig::NAVIGATION_SCAN_SET_MAX_AGE_MS-1;
    r.step(RobotConfig::AUTO_SCAN_RECENTER_SETTLE_MS);
    assert(r.nav.environmentalRecoveryInProgress()&&!r.nav.consumeEnvironmentalRecoveryCompleted()&&r.motors.motion==MotorState::Stopped);
  } else if(name=="ordinary_limit") {
    r.episode();r.nav._v2.commitSide(true);for(unsigned i=0;i<6;++i)assert(r.nav._v2.startTurn(6));
    r.nav.enterV2TrappedHold("ESCAPE_TURN_LIMIT",testNow);r.feed(UltrasonicResultState::DistanceValid,15);
    r.step(RobotConfig::RECOVERY_COOLDOWN_MS);for(unsigned i=0;i<30;++i){r.feed(UltrasonicResultState::DistanceValid,15);r.step(100);}
    assert(!r.nav._v2.watchdogExhausted()&&r.nav._v2.sideTurns()==6&&r.motors.motion==MotorState::Stopped);
  } else if(name=="watchdog_stop") {
    exhaust(r);r.nav.notifyOperatorStop();assert(!r.nav._v2.episodeActive()&&r.nav._operatorStopLatched);
    r.feed();r.step(RobotConfig::RECOVERY_COOLDOWN_MS+RobotConfig::AUTO_ENVIRONMENT_CLEAR_MS);
    assert(r.motors.motion==MotorState::Stopped&&r.nav._operatorStopLatched);
    ++r.web.modeCounter;r.feed();r.step();assert(!r.nav._operatorStopLatched&&!r.nav._v2.watchdogExhausted());
#ifdef CPS_COLLISION
  } else if(name=="watchdog_integration") {
    exhaust(r);const auto spent=r.nav._v2.activeRecoveryMs(testNow);provisional(r);
    authorization=false;r.step();r.step(10000);
    assert(r.nav._v2.watchdogExhausted()&&!r.nav._v2.handoffPending()&&r.nav._v2.activeRecoveryMs(testNow)==spent&&r.motors.motion==MotorState::Stopped);
    authorization=true;r.feed();r.step();assert(r.nav._v2.activeRecoveryMs(testNow)==spent&&r.motors.motion==MotorState::Stopped);
#endif
  } else if(name=="watchdog_wrap") {
    StuckRecoveryV2Policy p;p.noteBoxedIn();assert(p.startEscape(UINT32_MAX-10000));
    assert(!p.escapeTimedOut(19998,30000));assert(p.escapeTimedOut(19999,30000));p.exhaustWatchdog(19999);
    p.resume(22000);assert(p.activeRecoveryMs(23000)==30000);
    assert(p.beginWatchdogReassessment(22499,2500));p.handoffCandidate(22500);
    p.noteSustainedClear(4999);assert(p.episodeActive());p.noteSustainedClear(5000);assert(!p.episodeActive());
  } else assert(!"Unknown closure history");
  std::printf("PASS closure %s\n",argv[1]);
}
