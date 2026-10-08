"""Real Chromium executes current embedded mode/STOP/lifecycle producers; HTTP doubled."""
from pathlib import Path
import json,os,shutil,socket,subprocess,tempfile,time,unittest,urllib.request
ROOT=Path(__file__).resolve().parents[1]
class V46Browser(unittest.TestCase):
 def test_v46_16_17_actual_browser_stop_mode_barrier_and_lifecycle(self):
  candidates=[os.environ.get('PIKU_HEADLESS_BROWSER',''),r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe',r'C:\Program Files\Google\Chrome\Application\chrome.exe',shutil.which('chromium')]
  browser=next((str(p) for p in candidates if p and Path(p).is_file()),None)
  if not browser or not shutil.which('deno'):raise RuntimeError('Existing Chromium/Deno are required; no skip fallback')
  with tempfile.TemporaryDirectory(prefix='piku-v46-browser-') as directory:
   work=Path(directory);payload=work/'input.json';payload.write_text(json.dumps({'assets':(ROOT/'src/WebDashboardAssets.cpp').read_text(encoding='utf-8-sig')}),encoding='utf-8')
   with socket.socket() as listener:listener.bind(('127.0.0.1',0));port=listener.getsockname()[1]
   process=subprocess.Popen([browser,'--headless=new','--disable-gpu','--no-first-run','--no-default-browser-check','--disable-background-networking','--remote-debugging-address=127.0.0.1','--remote-debugging-port='+str(port),'--user-data-dir='+str(work/'profile'),'about:blank'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
   try:
    for _ in range(100):
     try:
      with urllib.request.urlopen('http://127.0.0.1:'+str(port)+'/json/list',timeout=1) as response:target=next(t for t in json.load(response) if t['type']=='page')
      break
     except (OSError,StopIteration):time.sleep(.1)
    else:raise RuntimeError('Chromium startup timed out')
    result=subprocess.run(['deno','run','--allow-read='+str(work),'--allow-net=127.0.0.1',str(ROOT/'tests/v46_browser_race.js'),target['webSocketDebuggerUrl'],str(payload)],capture_output=True,text=True,timeout=60)
    self.assertEqual(result.returncode,0,result.stdout+result.stderr)
    observed=json.loads(result.stdout);self.assertFalse(observed['ordered']['earlyMode']);self.assertEqual(observed['lifecycle'],3)
    self.assertTrue(observed['ordered']['maintenanceTagged']);self.assertTrue(observed['manualLifecycle'])
   finally:process.terminate();process.wait(timeout=10)
if __name__=='__main__':unittest.main()
