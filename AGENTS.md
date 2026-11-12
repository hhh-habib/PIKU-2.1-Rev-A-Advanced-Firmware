# PIKU 2.1 Rev A standalone repository

Read README.md, docs/README.md and docs/REPRODUCIBILITY.md before changes.

- Preserve Robot-local navigation/motor/safety authority and frozen dated evidence. Do not flash hardware or change firmware behavior without authorization.
- Build: `pio run -e esp32-s3-n16r8`.
- Full-clone checks: `python -B tools/verify_release.py`. Historical generic failures and candidate exclusions are documented; do not remove assertions or silently add skips.
- This checkpoint intentionally tracks owner-authorized robot SoftAP defaults in include/Secrets.h. Preserve its bytes unless configuration edits are requested. Do not print credential values in reports or add personal home/cloud credentials.
- Preserve original DOCX/XLSX/images, manifests and dated validation records. A code/build pass does not identify a physical experiment's flashed image.
- Stage an explicit reviewed allowlist. Do not publish unrelated branches/tags, generated firmware, machine state, private archives or large original video.
- On the owner's canonical Development workstation follow E:/Development/AGENTS.md and record meaningful authorized work through Research Continuity V1. Workstation paths are not build prerequisites for a GitHub clone.
