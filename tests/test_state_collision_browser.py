"""Rendered current Standalone UI using the established headless regression scenarios."""
from pathlib import Path
import re,json,os,shutil,socket,tempfile,time,urllib.request,subprocess,unittest
ROOT=Path(__file__).resolve().parents[1]
ASSETS=(ROOT/'src/WebDashboardAssets.cpp').read_text(encoding='utf-8-sig')
BEFORE=subprocess.check_output(['git','show','8f294741cb736fa77274250007f77429570ab978:src/WebDashboardAssets.cpp'],cwd=ROOT).decode('utf-8-sig')
FROZEN=BEFORE
class RenderedDashboardTests(unittest.TestCase):
    """Requires Deno and a headless Chromium browser; missing tools fail, not skip.

    Set PIKU_HEADLESS_BROWSER for a custom executable and PIKU_FROZEN_UI_REPO
    for a relocated read-only standalone repository. No robot/network fixtures.
    """
    @classmethod
    def setUpClass(cls):
        candidates = [os.environ.get('PIKU_HEADLESS_BROWSER', ''),
                      r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe',
                      r'C:\Program Files\Google\Chrome\Application\chrome.exe',
                      shutil.which('chromium'), shutil.which('google-chrome')]
        browser = next((str(p) for p in candidates if p and Path(p).is_file()), None)
        if not browser:
            raise RuntimeError('Headless Chromium required; set PIKU_HEADLESS_BROWSER.')
        if not shutil.which('deno'):
            raise RuntimeError('Deno is required for computed-style dashboard tests.')
        with tempfile.TemporaryDirectory(prefix='piku-dashboard-rendered-') as directory:
            work = Path(directory)
            screenshot_dir = os.environ.get('PIKU_DASHBOARD_SCREENSHOT_DIR')
            if screenshot_dir:
                Path(screenshot_dir).mkdir(parents=True, exist_ok=True)
            payload = work / 'input.json'
            payload.write_text(json.dumps({'baseline': BEFORE, 'current': ASSETS, 'frozen': FROZEN,
                                           'screenshotDir': screenshot_dir}), encoding='utf-8')
            with socket.socket() as listener:
                listener.bind(('127.0.0.1', 0))
                port = listener.getsockname()[1]
            process = subprocess.Popen([browser, '--headless=new', '--disable-gpu', '--no-first-run',
                '--no-default-browser-check', '--disable-background-networking',
                '--remote-debugging-address=127.0.0.1', '--remote-debugging-port=' + str(port),
                '--user-data-dir=' + str(work / 'profile'), 'about:blank'],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
            try:
                for attempt in range(100):
                    try:
                        with urllib.request.urlopen('http://127.0.0.1:' + str(port) + '/json/list', timeout=1) as response:
                            target = next(t for t in json.load(response) if t['type'] == 'page')
                        break
                    except (OSError, StopIteration):
                        time.sleep(.1)
                else:
                    raise RuntimeError('Headless browser startup timed out.')
                command = ['deno', 'run', '--allow-read=' + str(work), '--allow-net=127.0.0.1']
                if screenshot_dir:
                    command.append('--allow-write=' + screenshot_dir)
                command.extend([str(ROOT / 'tests/dashboard_browser_check.js'), target['webSocketDebuggerUrl'], str(payload)])
                result = subprocess.run(command, capture_output=True, text=True, timeout=120)
                if result.returncode:
                    raise RuntimeError(result.stderr)
                cls.observations = json.loads(result.stdout)
                for variant in ['frozen', 'baseline', 'current']:
                    observed = cls.observations[variant]
                    if len(observed['controls']) != 66 or len(observed['pages']) != 44:
                        raise RuntimeError('Incomplete rendered scenario coverage: ' + variant)
                    widths = {case['width'] for case in observed['controls']}
                    if widths != {360, 375, 390, 412, 430, 1180}:
                        raise RuntimeError('Missing required computed-style viewport: ' + variant)
            finally:
                if process.poll() is None:
                    process.terminate()
                process.wait(timeout=10)

    def cases(self, states=None):
        for observation in self.observations['current']['controls']:
            if states is None or observation['state'] in states:
                yield observation

    def test_alarm_computed_grid_has_three_equal_columns_and_one_row(self):
        for case in self.cases():
            with self.subTest(width=case['width'], height=case['height'], state=case['state']):
                columns = [float(c[:-2]) for c in case['gridColumns'].split()]
                self.assertEqual(len(columns), 3)
                self.assertLess(max(columns) - min(columns), .05)
                self.assertEqual(case['gridGap'], '8px')
                self.assertEqual(len(case['buttons']), 3)
                self.assertLess(max(b['y'] for b in case['buttons']) - min(b['y'] for b in case['buttons']), .05)
                self.assertEqual([b['id'] for b in case['buttons']], ['enableAlarmBtn', 'testBeepBtn', 'muteAlarmBtn'])

    def test_manual_normal_ready_and_directions_enabled(self):
        for case in self.cases(['normal']):
            self.assertEqual(case['status'], 'Manual controls ready.')
            self.assertTrue(all(not b['disabled'] and b['aria'] == 'false' for b in case['holds']))
            self.assertFalse(case['stopDisabled'])

    def test_manual_hazard_paused_directions_disabled_stop_enabled(self):
        for case in self.cases(['flame-blocked', 'gas-blocked']):
            self.assertEqual(case['status'], 'Manual controls paused.')
            self.assertTrue(all(b['disabled'] and b['aria'] == 'true' for b in case['holds']))
            self.assertFalse(case['stopDisabled'])
            self.assertFalse(case['overrideHidden'])
            self.assertFalse(case['alertHidden'])
            self.assertEqual(case['overrideText'], 'Enable Override')
            self.assertEqual(case['alertText'], 'Flame/Gas Hazard Detected - Override disabled')

    def test_override_active_exact_button_alert_and_critical_banner(self):
        for case in self.cases(['flame-override', 'gas-override']):
            self.assertEqual(case['status'], 'Manual controls ready.')
            self.assertTrue(all(not b['disabled'] for b in case['holds']))
            self.assertFalse(case['stopDisabled'])
            self.assertEqual(case['overrideText'], 'Disable Override')
            self.assertEqual(case['alertText'], 'Manual Override: ENABLED')
            self.assertEqual(case['bannerText'], 'HAZARD ACTIVE - OVERRIDE ON')
            self.assertEqual(case['bannerClass'], 'alarm-indicator critical')
            self.assertFalse(case['overrideHidden'])
            self.assertFalse(case['overrideDisabled'])
            self.assertFalse(case['alertHidden'])
            self.assertIn('MQ-2' if case['state'].startswith('gas') else 'Flame', case['bannerDetail'])

    def test_auto_compact_status_and_stop_enabled(self):
        for case in self.cases(['auto']):
            self.assertEqual(case['status'], 'AUTO mode active. STOP is available.')
            self.assertTrue(all(b['disabled'] for b in case['holds']))
            self.assertFalse(case['stopDisabled'])

    def test_shared_computed_styles_and_state_match_actual_frozen_assets(self):
        frozen = self.observations['frozen']['controls']
        for current, reference in zip(self.cases(), frozen):
            with self.subTest(width=current['width'], height=current['height'], state=current['state']):
                for key in ['gridColumns', 'gridGap', 'holds', 'status', 'overrideText', 'overrideHidden',
                            'overrideDisabled', 'alertText', 'alertHidden', 'bannerText', 'bannerDetail',
                            'bannerClass', 'bannerColor', 'sharedStyles']:
                    expected = reference[key]
                    if key == 'overrideText': expected = {'Turn Override Off':'Disable Override','Override Hazard & Control':'Enable Override'}.get(expected,expected)
                    if key == 'alertText': expected = {'HAZARD ACTIVE - OVERRIDE ON':'Manual Override: ENABLED','Flame/Gas Hazard Detected':'Flame/Gas Hazard Detected - Override disabled'}.get(expected,expected)
                    self.assertEqual(current[key], expected, key)

    def test_rendered_operator_order_and_phone_touch_clearance(self):
        for case in self.cases():
            with self.subTest(width=case['width'], height=case['height'], state=case['state']):
                for earlier, later in zip(case['order'], case['order'][1:]):
                    self.assertGreaterEqual(later['y'] + .05, earlier['bottom'], (earlier['selector'], later['selector']))
                self.assertEqual(case['problems'], [])
                self.assertEqual(case['errors'], [])

    def test_other_pages_and_bottom_navigation_have_no_new_visual_regression(self):
        for current, baseline in zip(self.observations['current']['pages'], self.observations['baseline']['pages']):
            with self.subTest(width=current['width'], height=current['height'], page=current['page']):
                self.assertFalse(current['overflow'])
                self.assertEqual(current['layout'], baseline['layout'])
                self.assertEqual(current['nav'], baseline['nav'])

    def test_lower_alarm_status_has_no_duplicate_operator_buttons(self):
        for page in self.observations['current']['pages']:
            self.assertEqual(page['alarmHeading'], 'Alarm status')
            self.assertEqual(page['alarmButtons'], 0)

    def test_rendered_sonar_valid_open_fault_stale(self):
        self.assertEqual(self.observations['current']['sonar'],
                         [['42 cm', 'VALID'], ['OPEN', 'OPEN'], ['-- cm', 'FAULT'], ['-- cm', 'STALE']])

    def test_moved_controls_retain_alarm_and_override_requests(self):
        current, baseline = self.observations['current'], self.observations['baseline']
        self.assertEqual(current['alarms'], baseline['alarms'])
        self.assertEqual([r['url'] for r in current['alarms']], ['/alarm/enable', '/alarm/test', '/alarm/mute'])
        self.assertEqual(current['handlerRequests'], baseline['handlerRequests'])
        override = next(r for r in current['handlerRequests'] if r['url'] == '/manual-override')
        self.assertEqual(override['method'], 'POST')
        self.assertEqual(override['body'], 'set=on&token=mock-session&confirm=1')


if __name__ == '__main__':
    unittest.main()
