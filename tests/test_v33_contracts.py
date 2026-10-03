from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
def src(path):return (ROOT/path).read_text(encoding='utf-8-sig')
def body(text,start,end):return text.split(start,1)[1].split(end,1)[0]
class V33Contracts(unittest.TestCase):
 def test_permission_not_erased_by_ordinary_stop_or_session_reentry(self):
  main=src('src/main.cpp');web=src('src/WebDashboard.cpp')
  command=body(main,'void onDashboardCommandAccepted(','void onDashboardModeAccepted(')
  self.assertNotIn('revokeManualOverride',command)
  session=body(web,'void WebDashboard::handleSession()','void WebDashboard::handleSessionHeartbeat()')
  self.assertIn('applyCommand("RELEASE")',session);self.assertNotIn('_overrideActionCallback(false)',session)
  self.assertIn('!validSessionToken()',body(web,'bool WebDashboard::applyCommand(','void WebDashboard::'))
 def test_episode_budget_preserved_at_all_pause_checkpoints(self):
  nav=src('src/NavigationManager.cpp')
  for start,end in [('void NavigationManager::beginSonarReacquire(','void NavigationManager::updateSonarReacquire('),('void NavigationManager::updateV2RetryCooldown(','void NavigationManager::beginV2Escape('),('void NavigationManager::enterV2TrappedHold(','void NavigationManager::beginStuckRecovery(')]:
   checkpoint=body(nav,start,end)
   self.assertNotIn('_v2.abort()',checkpoint);self.assertNotIn('_v2.recovered()',checkpoint);self.assertNotIn('resetMission()',checkpoint)
  self.assertIn('_v2.handoffCandidate()',nav)
  self.assertIn('_v2.noteSustainedClear(_obstacleForwardClearMs)',nav)
 def test_diagnostics_are_robot_reported_on_existing_endpoint(self):
  web=src('src/WebDashboard.cpp')
  for field in ['episode','turns_used','turns_max','ir_pulses','sonar_hold','committed_side','last_transition']:self.assertIn(field,web)
if __name__=='__main__':unittest.main()
