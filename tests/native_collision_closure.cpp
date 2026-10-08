#define main previous_collision_main
#include "native_state_collisions.cpp"
#undef main
#define V32_SCENARIOS_LIBRARY
#include "native_v32_restoration.cpp"

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

// V4.5 migration of the historical watchdog/hold histories. Each named case
// executes the current production navigation, not the superseded proof ledger.
// Restored per-mission bounds replace V4.5 direction/lifetime ledgers.
void v32BudgetHistory(Rig&,const String& name) {
 if(name=="watchdog_wrap"){StuckRecoveryV2Policy p;p.noteBoxedIn();assert(p.startEscape(UINT32_MAX-10000));assert(!p.escapeTimedOut(19998,30000));assert(p.escapeTimedOut(19999,30000));return;}
 const char* scenario="timeout";
 if(name=="ordinary_limit"||name=="watchdog_full_ledger")scenario="exhaustion";
 if(name=="watchdog_proof")scenario="exit";
 if(name=="watchdog_stop")scenario="stop_recovery";
 if(name=="watchdog_ir")scenario="ir_turn";
 if(name=="watchdog_sonar")scenario="invalid";
 if(name=="watchdog_hazard")scenario="hazard";
 if(name=="watchdog_cycles")scenario="retry";
 restoredScenario(scenario);
}
void provisional(Rig& r){r.feed();r.step(RobotConfig::RECOVERY_COOLDOWN_MS);}
void exhaust(Rig& r,unsigned over=0,bool fullLedger=false){(void)over;(void)fullLedger;v32BudgetHistory(r,"watchdog_exact");}

int main(int argc, char** argv) {
  assert(argc == 2); String name=argv[1]; Rig r;
  if(name.substr(0,8)=="watchdog"||name=="ordinary_limit"){v32BudgetHistory(r,name);return 0;}
  if (name=="left" || name=="right" || name=="overlay_ir" || name=="overlay_verify" || name=="overlay_trial" || name=="overlay_hold" || name=="overlay_exhausted") {
    if(name=="overlay_exhausted")exhaust(r);
    else if(name.substr(0,7)=="overlay") {
      r.episode();
      if(name=="overlay_verify"){r.nav._v2.commitSide(true);assert(r.nav._v2.startTurn(6));r.nav._v2.turnVerified();}
      if(name=="overlay_trial")r.nav._v2.recovered();
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
  } else if(name=="env_ir_sonar") {
    r.environmental();r.safety.left=true;r.step();r.feed(UltrasonicResultState::SensorFault);r.step();
    assert(r.nav.environmentalRecoveryInProgress()&&r.motors.motion==MotorState::Stopped);
    r.restoreSonar();r.safety.left=false;r.feed();r.step();r.freshEnvironmentalScan();assert(r.motors.backwards==0);
  } else if(name=="env_invalid_completion") {
    r.environmental();scanUntil(r,NavigationManager::AutoPhase::RecenterSettle);
    r.nav._frontScan.ultrasonic.sampleMs=testNow-RobotConfig::NAVIGATION_SCAN_SET_MAX_AGE_MS-1;
    r.step(RobotConfig::AUTO_SCAN_RECENTER_SETTLE_MS);
    assert(r.nav.environmentalRecoveryInProgress()&&!r.nav.consumeEnvironmentalRecoveryCompleted()&&r.motors.motion==MotorState::Stopped);
#ifdef CPS_COLLISION
  } else if(name=="env_integration_ir") {
    r.environmental();authorization=false;r.step();r.safety.left=true;r.step(2000);
    authorization=true;r.feed();r.step();
    assert(r.nav.environmentalRecoveryInProgress()&&r.motors.motion==MotorState::Stopped);
    for(unsigned i=0;i<100;++i){r.feed();r.step(100);assert(r.nav.environmentalRecoveryInProgress()&&r.motors.motion==MotorState::Stopped);}
    r.safety.left=false;r.feed();r.step();r.freshEnvironmentalScan();assert(r.motors.backwards==0);
#endif
} else assert(!"Unknown closure history");
  std::printf("PASS closure %s\n",argv[1]);
}
