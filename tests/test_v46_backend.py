"""Actual Robot HTTP command/mode bodies + NavigationManager; server/callback bridge doubled."""
from pathlib import Path
import re,tempfile,unittest
import test_state_collisions as native
ROOT=Path(__file__).resolve().parents[1]
class V46Backend(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.fixture_dir=tempfile.TemporaryDirectory(prefix='piku-v46-backend-')
  source=(ROOT/'src/WebDashboard.cpp').read_text(encoding='utf-8-sig')
  bodies='\n'.join(native.declaration(source,marker) for marker in ('void WebDashboard::handleCommand(','void WebDashboard::applyMode(','bool WebDashboard::applyCommand('))
  for old,new in [('WebDashboard','BackendWeb'),('String','ControlString'),('ManualSensorHeadCommand','BackendHead'),('DriveCommandSource','BackendSource')]:bodies=re.sub(r'\b'+old+r'\b',new,bodies)
  cls.fixture=Path(cls.fixture_dir.name)/'backend.cpp';cls.fixture.write_text((ROOT/'tests/native_v46_backend.cpp').read_text().replace('// ACTUAL_BACKEND_BODIES',bodies),encoding='utf-8')
  native.StateCollisions.setUpClass.__func__(cls)
 @classmethod
 def tearDownClass(cls):native.StateCollisions.tearDownClass.__func__(cls);cls.fixture_dir.cleanup()
 case=native.StateCollisions.case
 def test_v46_16_reordered_manual_retirement_stop_cannot_latch_auto(self):self.case('reorder')
 def test_v46_17_genuine_auto_stop_still_wins_even_old_sequence(self):self.case('explicit')
 def test_manual_retirement_stop_and_origin_guard_remain(self):self.case('manual')
if __name__=='__main__':unittest.main()

