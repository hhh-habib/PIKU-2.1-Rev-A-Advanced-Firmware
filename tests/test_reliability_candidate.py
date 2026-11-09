"""Candidate regressions execute live source; historical assertions remain unchanged."""
from pathlib import Path
import unittest
import re
import tempfile
import json
import os
import shutil
import socket
import subprocess
import time
import urllib.request
import hashlib
import test_state_collisions as native

ROOT = Path(__file__).resolve().parents[1]

class CandidateScans(unittest.TestCase):
    fixture = ROOT / 'tests/native_reliability_candidate.cpp'
    setUpClass = classmethod(native.StateCollisions.setUpClass.__func__)
    tearDownClass = classmethod(native.StateCollisions.tearDownClass.__func__)
    case = native.StateCollisions.case

for scenario in ('good_good_timeout', 'timeout_good_good', 'good_timeout_good',
                 'one_good', 'all_invalid', 'open_quorum', 'mixed_no_quorum',
                 'expired', 'clock_wrap', 'older_trusted', 'fault_quorum',
                 'timeout_actual', 'orientation', 'generation', 'late',
                 'stop_scan', 'environment_scan', 'fault_scan', 'forward_untrusted_count'):
    setattr(CandidateScans, 'test_' + scenario,
            lambda self, scenario=scenario: self.case(scenario))

class CandidateModeFence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fixture_dir = tempfile.TemporaryDirectory(prefix='candidate-mode-', dir=ROOT/'.pio')
        source = (ROOT/'src/WebDashboard.cpp').read_text(encoding='utf-8-sig')
        markers = ('void WebDashboard::handleCommand(', 'void WebDashboard::applyMode(',
                   'bool WebDashboard::applyCommand(', 'void WebDashboard::handleMode(',
                   'bool WebDashboard::acceptModeFence(', 'void WebDashboard::sendOk(',
                   'void WebDashboard::handleModeAuto(', 'void WebDashboard::handleModeManual(',
                   'void WebDashboard::markCommandApplied(', 'void WebDashboard::invalidateControlSession(',
                   'void WebDashboard::handleSession(', 'bool WebDashboard::validSessionToken(',
                   'bool WebDashboard::sessionAlive(')
        bodies = '\n'.join(native.declaration(source, marker) for marker in markers)
        for old, new in [('WebDashboard', 'BackendWeb'), ('String', 'ControlString'),
                         ('ManualSensorHeadCommand', 'BackendHead')]:
            bodies = re.sub(r'\b'+old+r'\b', new, bodies)
        cls.fixture = Path(cls.fixture_dir.name)/'backend.cpp'
        cls.fixture.write_text((ROOT/'tests/native_mode_fence.cpp').read_text().replace('// ACTUAL_BACKEND_BODIES', bodies))
        native.StateCollisions.setUpClass.__func__(cls)

    @classmethod
    def tearDownClass(cls):
        native.StateCollisions.tearDownClass.__func__(cls)
        cls.fixture_dir.cleanup()

    case = native.StateCollisions.case

for scenario in ('normal', 'late_stop', 'expiry', 'release', 'safety', 'duplicate_apply',
                 'stale_stop', 'reboot', 'aliases', 'missing', 'origin', 'old_direction',
                 'late_manual', 'revoked_session', 'multiple_clients', 'session_replacement',
                 'session_sequence', 'stop_authority', 'stop_sequence_sync', 'revoked_recovery_proof'):
    setattr(CandidateModeFence, 'test_'+scenario,
            lambda self, scenario=scenario: self.case(scenario))

class CandidateBrowser(unittest.TestCase):
    def test_actual_browser_transactions_and_rendering(self):
        browser = next((p for p in [os.environ.get('PIKU_HEADLESS_BROWSER', ''),
            r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe',
            r'C:\Program Files\Google\Chrome\Application\chrome.exe'] if p and Path(p).is_file()), None)
        self.assertIsNotNone(browser, 'Chromium required')
        self.assertIsNotNone(shutil.which('deno'), 'Deno required')
        output = ROOT/'.pio/reliability-candidate-evidence'
        output.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='candidate-browser-', dir=ROOT/'.pio', ignore_cleanup_errors=True) as raw:
            work = Path(raw)
            payload = work/'input.json'
            frozen = subprocess.check_output(['git','show','fa375a7bf739fc306060048b500c2acf413db7a7:src/WebDashboardAssets.cpp'],cwd=ROOT).decode('utf-8-sig')
            from reconnect_backend_bridge import BackendBridge
            bridge = BackendBridge(CandidateModeFence)
            self.addCleanup(bridge.close)
            payload.write_text(json.dumps({'nativeEndpoint':bridge.endpoint,'assets':(ROOT/'src/WebDashboardAssets.cpp').read_text(),
                                         'baseline':frozen,'output':str(output)}),encoding='utf-8')
            with socket.socket() as listener:
                listener.bind(('127.0.0.1',0));port=listener.getsockname()[1]
            process=subprocess.Popen([browser,'--headless=new','--disable-gpu','--no-first-run',
                '--no-default-browser-check','--disable-background-networking',
                '--remote-debugging-address=127.0.0.1','--remote-debugging-port='+str(port),
                '--user-data-dir='+str(work/'profile'),'about:blank'],stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
            try:
                for _ in range(100):
                    try:
                        with urllib.request.urlopen('http://127.0.0.1:'+str(port)+'/json/list',timeout=1) as response:
                            target=next(t for t in json.load(response) if t['type']=='page')
                        break
                    except (OSError,StopIteration):time.sleep(.1)
                else:raise RuntimeError('Chromium startup timed out')
                run=subprocess.run(['deno','run','--no-config','--allow-read='+str(work),
                    '--allow-write='+str(output),'--allow-net=127.0.0.1',
                    str(ROOT/'tests/reliability_candidate_browser.js'),target['webSocketDebuggerUrl'],str(payload)],
                    capture_output=True,text=True,timeout=120)
                self.assertEqual(run.returncode,0,run.stdout+run.stderr)
                observed=json.loads(run.stdout)
                (output/'browser-results.json').write_text(json.dumps(observed,indent=2),encoding='utf-8')
                self.assertGreaterEqual(len(observed['behavior']),20)
            finally:
                if process.poll() is None:
                    try:process.wait(timeout=3)
                    except subprocess.TimeoutExpired:process.terminate();process.wait(timeout=10)

class CandidateReview(unittest.TestCase):
    def test_control_simplification_preserves_typography_and_layout(self):
        review=json.loads((ROOT/'tests/reliability_candidate_review.json').read_text())
        assets=(ROOT/'src/WebDashboardAssets.cpp').read_text()
        for tag, expected in review['control_simplification']['preserved_assets'].items():
            body=re.search('R"'+tag+r'\(([\s\S]*?)\)'+tag+'";',assets).group(1)
            self.assertEqual(hashlib.sha256(body.encode()).hexdigest(),expected,tag)

    def test_exact_reviewed_candidate_and_untouched_frozen_source(self):
        review=json.loads((ROOT/'tests/reliability_candidate_review.json').read_text())
        frozen=json.loads((ROOT/'docs/STANDALONE_FREEZE_20261008_MANIFEST.json').read_text())
        for path, evidence in review['production'].items():
            self.assertEqual(hashlib.sha256((ROOT/path).read_bytes()).hexdigest(),evidence['raw_sha256'],path)
        for path, evidence in frozen['production_configuration_files'].items():
            if path=='include/Secrets.h' or path in review['production']:continue
            self.assertEqual(hashlib.sha256((ROOT/path).read_bytes()).hexdigest(),evidence['raw_sha256'],path)

    def test_navigation_and_command_authority_delta_is_narrow(self):
        baseline='fa375a7bf739fc306060048b500c2acf413db7a7'
        def old(path):return subprocess.check_output(['git','show',baseline+':'+path],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
        path='src/NavigationManager.cpp';before=old(path);after=(ROOT/path).read_text()
        allowed={'recordScanReading','scanSampleTrusted','updateForward','beginScan','updateScan','beginSonarReacquire'}
        names=set(re.findall(r'NavigationManager::(\w+)\(',before))
        for name in names-allowed:
            self.assertEqual(native.declaration(after,'NavigationManager::'+name+'('),native.declaration(before,'NavigationManager::'+name+'('),name)
        before=old('src/WebDashboard.cpp');after=(ROOT/'src/WebDashboard.cpp').read_text()
        for name in ('handleCommand','applyMode'):
            self.assertEqual(native.declaration(after,'WebDashboard::'+name+'('),native.declaration(before,'WebDashboard::'+name+'('),name)
        addition='''    if (command == "STOP" || command == "RELEASE") {
      _modeFence.revoke();
      _modeFenceResponsePending = command == "STOP";
    }
'''
        body=native.declaration(after,'WebDashboard::applyCommand(')
        self.assertEqual(body.count(addition),1)
        renewal='''    // Fresh accepted commands from the current live owner are liveness too.
    // This cannot renew an expired/revoked session or one owned by another token.
    if (command != "STOP" && command != "RELEASE" && validSessionToken())
      _lastSessionHeartbeatMs = _lastCommandReceivedMs;
'''
        self.assertEqual(body.count(renewal),1)
        self.assertEqual(body.replace(addition,'').replace(renewal,''),native.declaration(before,'WebDashboard::applyCommand('))
        assets=(ROOT/'src/WebDashboardAssets.cpp').read_text();frozen=old('src/WebDashboardAssets.cpp')
        for name in ('initializeControls','initializeNavigation'):
            self.assertEqual(native.declaration(assets,'function '+name+'('),native.declaration(frozen,'function '+name+'('),name)

if __name__ == '__main__':
    unittest.main()
