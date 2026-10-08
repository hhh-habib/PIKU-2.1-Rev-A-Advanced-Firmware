from pathlib import Path
import unittest,hashlib,json,subprocess
from v46_approved_delta import before_v46
from v32_restoration_approved_delta import RESTORED
ROOT=Path(__file__).resolve().parents[1]
class V45Inventory(unittest.TestCase):
 def test_reviewed_current_source_delta(self):
  m=json.loads((ROOT/'tests/v45_reviewed_delta.json').read_text())
  names=subprocess.check_output(['git','-c','safe.directory='+str(ROOT),'diff','--name-only',m['baseline'],'--','src','include','platformio.ini'],cwd=ROOT,text=True).splitlines()
  semantic={n for n in names if n not in RESTORED and subprocess.check_output(['git','-c','safe.directory='+str(ROOT),'show',m['baseline']+':'+n],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')!=before_v46(n,(ROOT/n).read_text(encoding='utf-8-sig'))}
  self.assertEqual(semantic,set(m['reviewed_sha256'])-RESTORED)
  for n,h in m['reviewed_sha256'].items():
   if n in RESTORED:continue # validated by current restoration inventory/parity
   self.assertEqual(hashlib.sha256(before_v46(n,(ROOT/n).read_text(encoding='utf-8-sig')).encode()).hexdigest(),h,n)
