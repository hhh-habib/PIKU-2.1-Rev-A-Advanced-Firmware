"""Reviewed V4.5 projection for HISTORICAL STATIC guards; behavioral tests use live source."""
from pathlib import Path
import hashlib,json,subprocess
from v46_approved_delta import before_v46
from v32_restoration_approved_delta import restored_source
from reliability_candidate_review import before_candidate
ROOT=Path(__file__).resolve().parents[1]
REVIEW=json.loads((ROOT/'tests/v45_reviewed_delta.json').read_text())
def before_v45(path,text):
 text=before_candidate(path,text)
 if restored_source(path,text):
  return subprocess.check_output(['git','show',REVIEW['baseline']+':'+path],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
 text=before_v46(path,text).replace('\r\n','\n')
 expected=REVIEW['reviewed_sha256'].get(path)
 if expected is None:return text
 baseline=subprocess.check_output(['git','-c','safe.directory='+str(ROOT),'show',REVIEW['baseline']+':'+path],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
 if text==baseline:return text # idempotent historical projection
 assert hashlib.sha256(text.encode()).hexdigest()==expected,('unreviewed V4.5 delta',path)
 return subprocess.check_output(['git','-c','safe.directory='+str(ROOT),'show',REVIEW['baseline']+':'+path],cwd=ROOT).decode('utf-8-sig').replace('\r\n','\n')
