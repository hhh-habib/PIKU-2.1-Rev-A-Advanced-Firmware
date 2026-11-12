"""Run the unchanged candidate suite using a V3.2 reference from Git history.

Full regression verification requires a full Git clone. Source ZIP downloads can
build firmware without Git; historical comparison tests need the old commits.
No source files, frozen manifests, or historical test assertions are modified.
"""
from pathlib import Path
import argparse
import importlib.util
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REFERENCE_COMMIT = "93a44eff1445fb675fdf737645988436b1883cbc"


def prepare_reference():
    reference = ROOT / ".pio" / "release-reference-v32-93a44eff"
    reference.mkdir(parents=True, exist_ok=True)
    entries = subprocess.check_output(
        ["git", "--no-optional-locks", "ls-tree", "-r", "--name-only",
         REFERENCE_COMMIT, "--", "src", "include"], cwd=ROOT, text=True)
    for name in entries.splitlines():
        if name == "include/Secrets.h":
            continue
        target = reference / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(subprocess.check_output(
            ["git", "--no-optional-locks", "show", REFERENCE_COMMIT + ":" + name],
            cwd=ROOT))
    return reference


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default=".pio/release-verification",
                        help="New or existing evidence directory beneath .pio")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to((ROOT / ".pio").resolve()):
        parser.error("Evidence output must stay beneath the ignored .pio directory")
    output.mkdir(parents=True, exist_ok=True)
    if not shutil.which("git"):
        parser.error("Git and a full clone are required for historical regressions")
    try:
        subprocess.run(["git", "--no-optional-locks", "cat-file", "-e",
                        REFERENCE_COMMIT + "^{commit}"], cwd=ROOT,
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        parser.error("Historical commit missing: use a full clone, not a ZIP/shallow clone")
    if not (shutil.which("g++") or shutil.which("clang++")):
        core_roots = [Path(sys.executable).parents[2], Path.home() / ".platformio"]
        if os.environ.get("PLATFORMIO_CORE_DIR"):
            core_roots.insert(0, Path(os.environ["PLATFORMIO_CORE_DIR"]))
        compiler = next((p / "packages/toolchain-gccmingw32/bin/g++.exe"
                         for p in core_roots
                         if (p / "packages/toolchain-gccmingw32/bin/g++.exe").is_file()), None)
        if compiler:
            os.environ["PATH"] = str(compiler.parent) + os.pathsep + os.environ["PATH"]
        else:
            parser.error("Install a native C++ compiler (g++ or clang++), or expose it on PATH")
    # Always use this repository's immutable history unless the caller explicitly
    # requests another reference. No sibling checkout or Downloads path is needed.
    if not os.environ.get("PIKU_V32_REFERENCE"):
        os.environ["PIKU_V32_REFERENCE"] = str(prepare_reference())
    spec = importlib.util.spec_from_file_location(
        "piku_candidate_verification", ROOT / "tools/verify_reliability_candidate.py")
    runner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(runner)
    runner.OUTPUT = output
    return runner.main()


if __name__ == "__main__":
    sys.exit(main())
