"""V4.5 histories execute the actual NavigationManager against hardware doubles."""
import unittest
import test_state_collisions as native
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class V45Navigation(unittest.TestCase):
    fixture = ROOT/'tests/native_v45_navigation.cpp'
    setUpClass = classmethod(native.StateCollisions.setUpClass.__func__)
    tearDownClass = classmethod(native.StateCollisions.tearDownClass.__func__)
    case = native.StateCollisions.case

    def test_n01_single_ir_reverse_and_full_scan(self): self.case('single')
    def test_n03_finite_low_side_rejected(self): self.case('boxed')
    def test_n05_ir_cancels_turn_then_front_and_side(self): self.case('resume')
    def test_n06_reference_front_clear_exit(self): self.case('clear')
    def test_n07_seventh_turn_exhaustion_hold(self): self.case('sixth')
    def test_n15_both_reverse_full_scan(self): self.case('both')
    def test_n16_accepted_normal_scan_finishes(self): self.case('accepted')
    def test_n15_stop_mode_preserve_held_both_episode(self): self.case('both_stop')
    def test_n15_boxed_transfer_completion_preserves_both_latch(self): self.case('both_transfer')
    def test_n15_required_holds_preserve_once_only_normal_both(self): self.case('both_holds')
    def test_n11_retired_sample_and_ready_turn_cannot_resurrect_after_stop(self): self.case('retired_work')
    def test_n02_held_repeat_opposite_and_both_checkpoint(self): self.case('normal_checkpoint')
    def test_n03_scan_validity_and_diagnostic_history(self): self.case('scan_validity')
    def test_n04_chosen_side_low_open_invalid(self): self.case('chosen_side')
    def test_n08_clear_sixth_normal_forward_ir_at_1_and_1000ms(self): self.case('handoff')
    def test_n09_n21_reference_right_ir_selection(self): self.case('direction')
    def test_n10_reference_elapsed_timeout_precedence(self): self.case('boundary')
    def test_n11_stop_mode_critical_sonar_front_completion_races(self): self.case('safety_races')
    def test_n12_reboot_has_no_old_action_or_run(self): self.case('boot')
    def test_n13_n23_environmental_seam_and_rotated_geometry(self): self.case('environment')
    def test_n17_interrupted_turn_is_not_replayed(self): self.case('sixth_resume')
    def test_n18_reference_wall_time_bound(self): self.case('clock_pause')
    def test_n19_saved_regular_front_chosen_side_and_completed_turn(self): self.case('saved_phases')
    def test_n19_reference_open_front_exit(self): self.case('incidental')
    def test_n24_automatic_retry_resets_mission(self): self.case('ten_pulses')


if __name__ == '__main__': unittest.main()
