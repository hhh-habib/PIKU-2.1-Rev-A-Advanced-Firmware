"""Hash-pinned navigation restoration recognition for historical STATIC projections only."""
from pathlib import Path
import json,hashlib
ROOT=Path(__file__).resolve().parents[1]
REVIEW=json.loads((ROOT/'tests/v32_restoration_reviewed_delta.json').read_text())
RESTORED=set(REVIEW['normalized_sha256'])
def restored_source(path,text):
 if path not in RESTORED:return False
 assert hashlib.sha256(text.replace('\r\n','\n').encode()).hexdigest()==REVIEW['normalized_sha256'][path],('unreviewed restoration source',path)
 return True
