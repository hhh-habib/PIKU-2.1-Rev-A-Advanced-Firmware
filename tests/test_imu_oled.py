"""Compile the actual IMU/OLED sources with deterministic bus/display doubles.

These tests verify logic and fault transitions, not electrical reliability or RF.
Historical frozen identities remain independently checked by the candidate suite.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest
import hashlib
import json
import test_state_collisions as native

ROOT = Path(__file__).resolve().parents[1]


class ImuOled(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix='imu-oled-', dir=ROOT/'.pio')
        work = Path(cls.directory.name)
        shutil.copyfile(ROOT/'tests/imu_oled_arduino.h', work/'Arduino.h')
        shutil.copyfile(ROOT/'tests/imu_oled_wire.h', work/'Wire.h')
        shutil.copyfile(ROOT/'include/DisplayManager.h', work/'DisplayManager.h')
        data = native.declaration((ROOT/'include/WebDashboard.h').read_text(), 'struct DashboardData')
        (work/'WebDashboard.h').write_text('#pragma once\n#include "Arduino.h"\n'+data+'''
class WebDashboard {public:
  struct {bool staConnected=false;String staIp="0.0.0.0";} _wifiTelemetry;
  void copyDisplayNetworkState(DashboardData&) const;
};
''')
        (work/'SPI.h').write_text('#pragma once\nstruct SpiDouble {}; extern SpiDouble SPI;\n')
        (work/'Adafruit_SH110X.h').write_text('''#pragma once
#include "Arduino.h"
#define SH110X_WHITE 1
struct OledState {unsigned frames=0;String text;}; extern OledState oledState;
class Adafruit_SH1106G {public:
  template<class T>Adafruit_SH1106G(int,int,T*,int,int,int){}
  bool begin(int,bool){return true;}void setRotation(int){}void setTextSize(int){}
  void setTextColor(int){}void setCursor(int,int){}void clearDisplay(){oledState.text.clear();}
  void display(){++oledState.frames;}void println(const char* s){oledState.text+=s;oledState.text+='\\n';}
  void print(float,int){}void print(const char*){}
};
''')
        cc = shutil.which('g++') or str(Path.home()/'.platformio/packages/toolchain-gccmingw32/bin/g++.exe')
        cls.env = os.environ.copy()
        cls.env.update(PATH=str(Path(cc).parent)+os.pathsep+cls.env.get('PATH',''), TMP=str(work), TEMP=str(work))
        cls.exe = work/'imu-oled.exe'
        bridge = native.declaration((ROOT/'src/WebDashboard.cpp').read_text(),
                                    'void WebDashboard::copyDisplayNetworkState(')
        (work/'native.cpp').write_text((ROOT/'tests/native_imu_oled.cpp').read_text()+'\n'+bridge)
        command = [cc, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I'+str(work), '-I'+str(ROOT/'include'),
                   str(work/'native.cpp'), str(ROOT/'src/ImuManager.cpp'),
                   str(ROOT/'src/DisplayManager.cpp'), '-o', str(cls.exe)]
        result = subprocess.run(command, capture_output=True, text=True, env=cls.env)
        if result.returncode: raise RuntimeError(result.stdout+result.stderr)

    @classmethod
    def tearDownClass(cls): cls.directory.cleanup()

    def case(self, name):
        result = subprocess.run([str(self.exe), name], capture_output=True, text=True, env=self.env)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)


class ImuOledScope(unittest.TestCase):
    def test_protected_navigation_environment_network_and_radio_bytes(self):
        layer = json.loads((ROOT/'tests/reliability_candidate_review.json').read_text())['imu_oled_20261107']
        for path, expected in layer['protected_raw_sha256'].items():
            self.assertEqual(hashlib.sha256((ROOT/path).read_bytes()).hexdigest(), expected, path)

    def test_main_changes_only_sample_validity_age_and_motor_state_input(self):
        layer = json.loads((ROOT/'tests/reliability_candidate_review.json').read_text())['imu_oled_20261107']
        text = (ROOT/'src/main.cpp').read_bytes()
        text = text.replace(b'  dashboardData.imuHasSample = imuState.hasSample;\r\n', b'')
        text = text.replace(b'imuState.hasSample ? now - imuState.lastSuccessMs : 0',
                            b'sampleAge(imuState.lastSuccessMs, now)')
        text = text.replace(b'imu.update(motors.getState() == MotorState::Stopped);', b'imu.update();')
        self.assertEqual(hashlib.sha256(text).hexdigest(), layer['changed_files']['src/main.cpp']['before_raw_sha256'])

    def test_age_is_null_until_a_verified_sample_exists(self):
        text = (ROOT/'src/WebDashboard.cpp').read_text()
        self.assertIn('(_data.imuHasSample ? String(_data.imuSampleAgeMs) : String("null"))', text)
        self.assertIn("value === null || value === undefined ? '--'", (ROOT/'src/WebDashboardAssets.cpp').read_text())

    def test_network_handlers_and_environment_publisher_have_only_declared_bridges(self):
        layer = json.loads((ROOT/'tests/reliability_candidate_review.json').read_text())['imu_oled_20261107']
        edits = {
            'src/DashboardPublisher.cpp': [
                ('  dashboard.copyDisplayNetworkState(data); // Existing cache only; no Wi-Fi polling.\r\n', '')],
            'include/WebDashboard.h': [
                ('  bool imuHasSample = false;\r\n', ''),
                ('  void copyDisplayNetworkState(DashboardData& target) const;\r\n', '')],
            'src/WebDashboard.cpp': [
                ('void WebDashboard::copyDisplayNetworkState(DashboardData& target) const {\r\n'
                 '  target.staConnected = _wifiTelemetry.staConnected;\r\n'
                 '  target.staIp = _wifiTelemetry.staIp;\r\n}\r\n\r\n', ''),
                ('(_data.imuHasSample ? String(_data.imuSampleAgeMs) : String("null"))',
                 'String(_data.imuSampleAgeMs)')]
        }
        for path, replacements in edits.items():
            text = (ROOT/path).read_bytes()
            for after, before in replacements:
                self.assertEqual(text.count(after.encode()), 1, path)
                text = text.replace(after.encode(), before.encode())
            self.assertEqual(hashlib.sha256(text).hexdigest(), layer['changed_files'][path]['before_raw_sha256'], path)


for scenario in ('qualification', 'stationary', 'units', 'boot_retry', 'address69', 'wrong_identity',
                 'short_read', 'bus_error', 'zero_frame', 'ff_frame', 'runtime_recovery',
                 'moving_init', 'moving_mid_init', 'no_data_ready', 'config_mismatch',
                 'reset_timeout', 'backoff', 'clock_wrap', 'zero_timestamp', 'oled_normal',
                 'oled_unchanged', 'oled_radio_sta', 'oled_alerts', 'oled_distance', 'oled_invalid', 'oled_bridge'):
    setattr(ImuOled, 'test_'+scenario, lambda self, scenario=scenario: self.case(scenario))

if __name__ == '__main__': unittest.main()
