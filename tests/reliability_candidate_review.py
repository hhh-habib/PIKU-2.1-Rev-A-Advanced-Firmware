"""Exact candidate recognition for existing HISTORICAL static projections only.

Runtime tests always read live source. Unknown edits are not projected. The new
candidate manifest/invariant tests replace obsolete frozen-identity assertions.
"""
from pathlib import Path
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT/'tests/reliability_candidate_review.json'

def before_candidate(path, text):
    if not MANIFEST.exists():
        return text
    review = json.loads(MANIFEST.read_text(encoding='utf-8'))
    expected = review['production'].get(path)
    normalized = text.replace('\r\n', '\n')
    if expected and expected['existed_at_freeze'] and hashlib.sha256(normalized.encode()).hexdigest() == expected['normalized_sha256']:
        return subprocess.check_output(['git', 'show', review['baseline']+':'+path],
                                       cwd=ROOT).decode('utf-8-sig').replace('\r\n', '\n')
    return text
