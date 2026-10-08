"""Live V4.6 inventory, exact BOXED lock, and unchanged production authorities."""
from pathlib import Path
import hashlib,json,unittest
from test_state_collisions import declaration
from v32_restoration_approved_delta import RESTORED,REVIEW as RESTORATION
ROOT=Path(__file__).resolve().parents[1]
M=json.loads((ROOT/'tests/v46_reviewed_delta.json').read_text())
START=json.loads((ROOT/'tests/v46_starting_source.json').read_text())
LOCKED=['beginV2Escape','selectRecoveryDirection','beginRecoverySideCheck','finishRecoverySideCheck','beginRecoveryFrontCheck','updateRecoveryFrontCheck','beginRecoveryIrBackoff','updateRecoveryIrBackoff','v2ImuReady','beginV2Turn','enterV2TrappedHold','retireRecoveryRound','completeBoxed','resumeRecoveryPhase','scanSampleTrusted','recoveryReading','requestedSample','updateV2RetryCooldown','completeSpin','updateSpinTurn','finishRecoveryAfterSettle','updateEnvironmentalRecovery','completeEnvironmentalRecovery','selectClearestSide','beginScan','commandCurrentScanDirection','updateScan','recordScanReading','finalizeCurrentScan','storeCurrentScan','beginRecenter']
class V46Inventory(unittest.TestCase):
 def test_exact_current_scope_and_hashes(self):
  for path,h in M['reviewed_sha256'].items():
   if path in RESTORED:continue
   self.assertEqual(hashlib.sha256((ROOT/path).read_text(encoding='utf-8-sig').encode()).hexdigest(),h,path)
  for path,h in M['locked_production_raw_sha256'].items():
   if path in RESTORED:continue
   self.assertEqual(hashlib.sha256((ROOT/path).read_bytes()).hexdigest(),h,path)
  for path,data in START.items():self.assertEqual(hashlib.sha256(data['text'].encode()).hexdigest(),data['normalized_sha256'],path)
 def test_restoration_source_and_preserved_infrastructure(self):
  for path,h in RESTORATION['normalized_sha256'].items():self.assertEqual(hashlib.sha256((ROOT/path).read_text(encoding='utf-8-sig').encode()).hexdigest(),h,path)
  for path,h in RESTORATION['preserved_raw_sha256'].items():self.assertEqual(hashlib.sha256((ROOT/path).read_bytes()).hexdigest(),h,path)
 def test_approved_environmental_sonar_job_is_not_completed_by_fault(self):
  current=(ROOT/'src/NavigationManager.cpp').read_text(encoding='utf-8-sig')
  begin=declaration(current,'NavigationManager::beginSonarReacquire(')
  self.assertIn('_v2.abort()',begin);self.assertIn('_recoveryIrBackoff.resetMission()',begin)
  self.assertNotIn('_environmentalRecoveryActive = false',begin);self.assertIn('_environmentalRecoveryCompleted = false',begin)
  restore=declaration(current,'NavigationManager::updateSonarReacquire(')
  self.assertIn('beginScan(ScanPurpose::EnvironmentalRecovery, now)',restore)
 def test_explicit_stop_and_atomic_mode_backend_are_unchanged(self):
  before=START['src/WebDashboard.cpp']['text'];current=(ROOT/'src/WebDashboard.cpp').read_text(encoding='utf-8-sig')
  for name in ('applyCommand','applyMode'):
   self.assertEqual(declaration(current,'WebDashboard::'+name+'('),declaration(before,'WebDashboard::'+name+'('))
