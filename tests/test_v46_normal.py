"""V4.6 NORMAL histories execute live NavigationManager; hardware calls doubled."""
from pathlib import Path
import unittest
import test_state_collisions as native
ROOT=Path(__file__).resolve().parents[1]
class V46Normal(unittest.TestCase):
 fixture=ROOT/'tests/native_v46_normal.cpp'
 setUpClass=classmethod(native.StateCollisions.setUpClass.__func__)
 tearDownClass=classmethod(native.StateCollisions.tearDownClass.__func__)
 case=native.StateCollisions.case
 def test_v46_01_02_11_single_ir_reverse_full_scan(self):self.case('single')
 def test_v46_03_post_single_blocked_backoff_scan(self):self.case('post_blocked')
 def test_v46_04_05_06_front_reverse_full_scan(self):self.case('front')
 def test_v46_07_08_09_both_no_duplicate_pulse(self):self.case('both')
 def test_v46_10_12_opposite_and_escape_reset(self):self.case('opposite')
 def test_v46_13_scan_boxed_once(self):self.case('boxed')
 def test_v46_14_no_path_stop_wait_scan(self):self.case('no_path')
 def test_v46_15_fresh_auto_entry_discards_normal_history(self):self.case('entry')
 def test_v46_18_long_autonomous_actual_scan_history(self):self.case('long')
 def test_v46_19_normal_transactions_stop_mode_hazard_sonar_authority(self):self.case('races')
 def test_v46_20_ir_requires_fresh_front_side_instead_of_saved_turn(self):self.case('normal_boxed_lock')
 def test_v46_19_critical_stop_during_recovery(self):self.case('imu_bounds')
if __name__=='__main__':unittest.main()
