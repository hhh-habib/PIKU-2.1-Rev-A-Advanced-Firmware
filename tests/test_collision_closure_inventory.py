"""Pin the reviewed current production delta independently of historical gates."""
from pathlib import Path
import hashlib,json,subprocess,unittest
ROOT=Path(__file__).resolve().parents[1]
from v45_approved_delta import before_v45
class ClosureInventory(unittest.TestCase):
 def test_exact_current_production_delta_and_baseline(self):
  m=json.loads((ROOT/'tests/collision_closure_reviewed_delta.json').read_text())
  names=subprocess.check_output(['git','diff','--name-only',m['baseline'],'--','src','include','platformio.ini'],cwd=ROOT,text=True).splitlines()
  semantic=[]
  for name in names:
   old=subprocess.check_output(['git','show',m['baseline']+':'+name],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
   if old!=before_v45(name,(ROOT/name).read_text(encoding='utf-8-sig')):semantic.append(name)
  self.assertEqual(set(semantic),set(m['reviewed_sha256']))
  for name,expected in m['reviewed_sha256'].items():
   self.assertEqual(hashlib.sha256(before_v45(name,(ROOT/name).read_text(encoding='utf-8-sig')).encode()).hexdigest(),expected,name)
