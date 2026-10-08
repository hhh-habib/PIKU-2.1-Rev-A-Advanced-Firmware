"""Hash-pinned V4.6-to-live-V4.5 projection for historical STATIC guards only."""
from pathlib import Path
import hashlib,json,subprocess
from v32_restoration_approved_delta import restored_source
ROOT=Path(__file__).resolve().parents[1]
REVIEW=json.loads((ROOT/'tests/v46_reviewed_delta.json').read_text(encoding='utf-8-sig'))
START=json.loads((ROOT/'tests/v46_starting_source.json').read_text(encoding='utf-8-sig'))
def before_v46(path,text):
 text=text.replace('\r\n','\n')
 if path not in START:return text
 prior=START[path]['text']
 if restored_source(path,text):return prior
 if text==prior:return text
 baseline=subprocess.check_output(['git','-c','safe.directory='+str(ROOT),'show',REVIEW['starting_head']+':'+path],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
 if text==baseline:return text # historical layers can already have projected Git HEAD
 assert hashlib.sha256(text.encode()).hexdigest()==REVIEW['reviewed_sha256'][path],('unreviewed V4.6 delta',path)
 return prior
