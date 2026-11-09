"""Execute live HTTP, alert and reconnect bodies with deterministic hardware doubles.

No simulated clock result is a physical latency measurement. Socket/parser and
OLED doubles deliberately expose slow-client and busy-output interruptions.
"""
from pathlib import Path
import os
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
import unittest
import test_state_collisions as native
import test_reliability_candidate as candidate

ROOT = Path(__file__).resolve().parents[1]


class ResponseBodies(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix='response-bodies-', dir=ROOT/'.pio')
        work = Path(cls.directory.name)
        arduino = '''#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <string>
using String=std::string; using std::isfinite;
#define HIGH 1
#define LOW 0
unsigned long testNow=10000;
inline unsigned long millis(){return testNow;}
inline void yield(){++testNow;}
inline void delay(unsigned long n){testNow+=n;}
'''
        (work/'Arduino.h').write_text(arduino)
        types = '#pragma once\n#include "Arduino.h"\n'
        for file, markers in [('SensorManager.h', ['enum class UltrasonicResultState',
                'enum class FlameState', 'struct SensorSnapshot']),
                ('WebDashboard.h', ['struct DashboardData'])]:
            text = (ROOT/'include'/file).read_text()
            for marker in markers:
                types += native.declaration(text, marker)+'\n'
        (work/'ResponseTypes.h').write_text(types)
        source_root = Path(os.environ.get('PIKU_RESPONSE_BODY_SOURCE', str(ROOT)))
        display = (source_root/'src/DisplayManager.cpp').read_text()
        bodies = ''
        if 'uint8_t environmentalAlertMask(' in display:
            bodies += native.declaration(display, 'uint8_t environmentalAlertMask(')+'\n'
            bodies += native.declaration(display, 'void formatDisplayLines(')+'\n'
        bodies += native.declaration(display, 'void DisplayManager::updateDashboard(')
        bodies += '\n'+native.declaration(display, 'void DisplayManager::renderAlert(').replace(
            'DisplayManager::renderAlert(', 'DisplayManager::renderAlertContents(')
        marker = 'bool DisplayManager::alertUpdatePending('
        bodies += '\n'+(native.declaration(display, marker) if marker in display else
            'bool DisplayManager::alertUpdatePending(const DashboardData&) const { return false; }')
        publisher = (source_root/'src/DashboardPublisher.cpp').read_text()
        bodies += '\n'+native.declaration(publisher, 'const char* ultrasonicStateName(')
        bodies += '\n'+native.declaration(publisher, 'const char* flameStateName(')
        body = native.declaration(publisher, 'void DashboardPublisher::publishSensorReadings(')
        signature = body[:body.index('{')].replace('DashboardPublisher::', '')+';'
        network = (source_root/'src/WebDashboard.cpp').read_text()
        bodies += '\n'+body+'\n'+native.declaration(network, 'void WebDashboard::updateWiFi(')
        bodies += '\n'+native.declaration(network, 'void WebDashboard::setStaStateFromWiFiStatus(')
        fixture = (ROOT/'tests/native_standalone_response.cpp').read_text()
        fixture = fixture.replace('// PUBLISH_SIGNATURE', signature).replace('// LIVE_BODIES', bodies)
        (work/'response.cpp').write_text(fixture)
        compiler = shutil.which('g++') or str(Path.home()/'.platformio/packages/toolchain-gccmingw32/bin/g++.exe')
        cls.exe = work/'response.exe'
        env = os.environ.copy()
        env['PATH'] = str(Path(compiler).parent)+os.pathsep+env.get('PATH', '')
        cls.env = env
        result = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
            '-I'+str(work), '-I'+str(ROOT/'include'), str(work/'response.cpp'), '-o', str(cls.exe)],
            capture_output=True, text=True, env=env)
        if result.returncode:
            raise RuntimeError(result.stdout+result.stderr)

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def case(self, name):
        result = subprocess.run([str(self.exe), name], capture_output=True, text=True, env=self.env)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)

    def test_alert_bypasses_recent_routine_refresh(self): self.case('alert_fast')
    def test_alert_reason_change_bypasses_refresh(self): self.case('alert_reason')
    def test_busy_publisher_services_new_alert(self): self.case('alert_busy')
    def test_busy_publisher_services_alert_clear(self): self.case('alert_clear')
    def test_unchanged_alert_does_not_flood_display(self): self.case('alert_repeat')
    def test_alert_override_does_not_claim_motors_stopped(self): self.case('alert_override')
    def test_inflight_association_is_not_restarted(self): self.case('wifi_pending')
    def test_expired_association_can_retry(self): self.case('wifi_expired')
    def test_success_resets_backoff(self): self.case('wifi_connected')
    def test_scan_does_not_compete_with_retry(self): self.case('wifi_scan')
    def test_terminal_connection_loss_ends_attempt(self): self.case('wifi_lost')


class ResponseSessions(unittest.TestCase):
    setUpClass = classmethod(candidate.CandidateModeFence.setUpClass.__func__)
    tearDownClass = classmethod(candidate.CandidateModeFence.tearDownClass.__func__)
    case = candidate.CandidateModeFence.case

    def test_accepted_own_held_commands_keep_session_alive(self): self.case('held_session')
    def test_stale_commands_cannot_renew_session(self): self.case('stale_session')
    def test_other_token_cannot_renew_session(self): self.case('other_session')
    def test_expired_session_is_not_resurrected(self): self.case('expired_session')


class ResponseHttp(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix='response-http-', dir=ROOT/'.pio')
        work = Path(cls.directory.name)
        (work/'Arduino.h').write_text('#pragma once\n#include <cstdint>\n#include <cstddef>\n#define HIGH 1\n#define LOW 0\n'
            'unsigned long testNow=10000; inline unsigned long millis(){return testNow;}'
            'inline void yield(){++testNow;} inline void delay(unsigned long n){testNow+=n;}\n')
        shutil.copy(ROOT/'tests/http_response_doubles.h', work/'WebServer.h')
        source_root = Path(os.environ.get('PIKU_RESPONSE_HTTP_SOURCE', str(ROOT)))
        shutil.copy(source_root/'include/PriorityWebServer.h', work/'PriorityWebServer.h')
        source = (source_root/'src/PriorityWebServer.cpp').read_text().replace('#include <sys/socket.h>', '')
        source = source.replace('::send(', 'testSend(').replace('::recv(', 'testRecv(')
        source += '\n'+(ROOT/'tests/native_http_response.cpp').read_text()
        (work/'http.cpp').write_text(source)
        compiler = shutil.which('g++') or str(Path.home()/'.platformio/packages/toolchain-gccmingw32/bin/g++.exe')
        cls.exe = work/'http.exe'
        env = os.environ.copy()
        env['PATH'] = str(Path(compiler).parent)+os.pathsep+env.get('PATH', '')
        cls.env = env
        result = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror',
            '-I'+str(work), '-I'+str(ROOT/'include'), str(work/'http.cpp'), '-o', str(cls.exe)],
            capture_output=True, text=True, env=env)
        if result.returncode: raise RuntimeError(result.stdout+result.stderr)

    @classmethod
    def tearDownClass(cls): cls.directory.cleanup()

    def case(self, name):
        result = subprocess.run([str(self.exe), name], capture_output=True, text=True, env=self.env)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)


for scenario in ('fragmented_header', 'fragmented_body', 'absolute_request_deadline',
        'oversized_body', 'duplicate_length', 'chunked', 'stalled_control',
        'stalled_flash', 'stalled_telemetry', 'flash_short_writes', 'reset_does_not_spin', 'complete_get'):
    setattr(ResponseHttp, 'test_'+scenario, lambda self, scenario=scenario: self.case(scenario))


class ResponseScope(unittest.TestCase):
    def test_navigation_sensing_safety_motor_and_private_profile_code_preserved(self):
        review = json.loads((ROOT/'tests/reliability_candidate_review.json').read_text())
        for path, expected in review['standalone_response_20261019']['protected_raw_sha256'].items():
            layer = review.get('imu_oled_20261107', {}).get('changed_files', {}).get(path)
            if layer:
                self.assertIn(path, ('src/main.cpp', 'src/ImuManager.cpp'))
                self.assertEqual(layer['before_raw_sha256'], expected, path)
                expected = layer['after_raw_sha256']
            self.assertEqual(hashlib.sha256((ROOT/path).read_bytes()).hexdigest(), expected, path)

    def test_only_declared_output_and_network_limits_change(self):
        before = subprocess.check_output(['git', '--no-optional-locks', 'show',
            'fa375a7bf739fc306060048b500c2acf413db7a7:include/RobotConfig.h'], cwd=ROOT).decode('utf-8-sig')
        def settings(text):
            return dict(re.findall(r'^\s*static constexpr.*?\s+(\w+)(?:\[[^\]]*\])?\s*=\s*(.*?);', text, re.M))
        expected = settings(before)
        expected.update(SERIAL_LOG_INTERVAL_MS='2000', OLED_REFRESH_INTERVAL_MS='500',
            WIFI_STA_CONNECT_WINDOW_MS='15000', HTTP_CONTROL_SEND_BUDGET_MS='25',
            HTTP_ASSET_SEND_BUDGET_MS='250', HTTP_MAX_HEADER_BYTES='2048', HTTP_MAX_BODY_BYTES='1024')
        self.assertEqual(settings((ROOT/'include/RobotConfig.h').read_text()), expected)


if __name__ == '__main__':
    unittest.main()
