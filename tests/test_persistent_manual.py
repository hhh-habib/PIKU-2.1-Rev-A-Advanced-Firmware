"""Execute actual main.cpp timing guards; browser tests cover response scheduling."""
from pathlib import Path
import tempfile
import unittest
import test_state_collisions as native

ROOT=Path(__file__).resolve().parents[1]

class ManualTimingGuard(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fixture_dir=tempfile.TemporaryDirectory(prefix='manual-timing-',dir=ROOT/'.pio')
        source=(ROOT/'src/main.cpp').read_text()
        admission=native.declaration(source,'bool handleManualControl() {')
        admission=admission[:admission.index('  ManualGateInput gate;')]+'  return true;\n}\n'
        stop=native.declaration(source,'void stopMotors() {')
        independent=native.declaration(source,'if (dashboard.getControlMode() == "MANUAL" &&\n      manualMovementActive &&')
        independent='bool independentDeadman(){bool manualDeadmanTriggered=false;\n'+independent+'\nreturn manualDeadmanTriggered;}'
        cls.fixture=Path(cls.fixture_dir.name)/'manual.cpp'
        cls.fixture.write_text((ROOT/'tests/native_manual_timing.cpp').read_text().replace('// ACTUAL_MANUAL_TIMING_BODIES',stop+'\n'+admission+'\n'+independent))
        native.StateCollisions.setUpClass.__func__(cls)

    @classmethod
    def tearDownClass(cls):
        native.StateCollisions.tearDownClass.__func__(cls)
        cls.fixture_dir.cleanup()

    case=native.StateCollisions.case
    def test_repeated_ten_second_refresh_and_release(self):self.case('ten_seconds')
    def test_exact_deadman_and_fresh_direction_without_mode_change(self):self.case('expiry_and_release')
    def test_stop_and_critical_interlocks(self):self.case('stop_and_critical')
    def test_independent_deadman_without_pending_command(self):self.case('independent_deadman')
