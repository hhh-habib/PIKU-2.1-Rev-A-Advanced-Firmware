"""Narrow closure regressions execute current production classes and dispatch bodies."""
from pathlib import Path
import unittest
import test_state_collisions as states

ROOT = Path(__file__).resolve().parents[1]

class ClosureNavigation(unittest.TestCase):
    fixture = ROOT / 'tests/native_collision_closure.cpp'
    setUpClass = classmethod(states.StateCollisions.setUpClass.__func__)
    tearDownClass = classmethod(states.StateCollisions.tearDownClass.__func__)
    case = states.StateCollisions.case

    def test_persistent_left_full_environmental_history(self): self.case('left')
    def test_persistent_right_full_environmental_history(self): self.case('right')
    def test_environmental_post_turn_ir(self): self.case('post_ir')
    def test_environmental_episode_overlay_ir(self): self.case('overlay_ir')
    def test_watchdog_reassessment_reachable(self): self.case('watchdog')
    def test_environmental_decision_ir_clear_requires_fresh_scan(self): self.case('decision_ir')
    def test_environmental_turn_ir_requires_fresh_scan(self): self.case('turn_ir')
    def test_environmental_both_ir_veto(self): self.case('both_ir')
    def test_environmental_post_turn_requires_requested_front_proof(self): self.case('post_front')
    def test_watchdog_exact_threshold_stop(self): self.case('watchdog_exact')
    def test_watchdog_over_threshold_stop(self): self.case('watchdog_over')
    def test_watchdog_resources_retained_until_sustained_proof(self): self.case('watchdog_proof')
    def test_watchdog_provisional_ir_interruption(self): self.case('watchdog_ir')
    def test_watchdog_provisional_sonar_interruption(self): self.case('watchdog_sonar')
    def test_watchdog_provisional_hazard_interruption(self): self.case('watchdog_hazard')
    def test_watchdog_provisional_obstruction_interruption(self): self.case('watchdog_block')
    def test_watchdog_repeated_distinct_episode_boundaries(self): self.case('watchdog_cycles')
    def test_watchdog_uint32_clock_wrap(self): self.case('watchdog_wrap')
    def test_watchdog_full_turn_pulse_ledger_can_renew_only_at_fresh_boundary(self): self.case('watchdog_full_ledger')
    def test_environmental_escape_verify_overlay_ir(self): self.case('overlay_verify')
    def test_environmental_forward_trial_entry_overlay_ir(self): self.case('overlay_trial')
    def test_environmental_trapped_hold_overlay_ir(self): self.case('overlay_hold')
    def test_environmental_watchdog_exhausted_overlay_ir(self): self.case('overlay_exhausted')
    def test_environmental_ir_sonar_restore_full_history(self): self.case('env_ir_sonar')
    def test_environmental_stale_front_cannot_complete(self): self.case('env_invalid_completion')
    def test_ordinary_turn_limit_does_not_get_watchdog_renewal(self): self.case('ordinary_limit')
    def test_watchdog_stop_requires_existing_mode_rearm(self): self.case('watchdog_stop')
    def test_watchdog_integration_pause_cannot_refill_or_resume_old_motion(self):
        if not (ROOT/'include/CoordinatorLossPolicy.h').exists():self.skipTest('CPS integration authority is absent from Standalone')
        self.case('watchdog_integration')

if __name__ == '__main__': unittest.main()
