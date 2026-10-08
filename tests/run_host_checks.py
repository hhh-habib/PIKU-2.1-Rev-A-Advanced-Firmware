"""Run standalone policy, Python, production-body, and embedded JS checks."""
from pathlib import Path
import os,re,shutil,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1]
compiler=shutil.which('g++') or shutil.which('clang++')
if not compiler:
 candidates=[Path(os.environ.get('PLATFORMIO_CORE_DIR',''))/'packages/toolchain-gccmingw32/bin/g++.exe',Path(sys.executable).parents[2]/'packages/toolchain-gccmingw32/bin/g++.exe',Path.home()/'.platformio/packages/toolchain-gccmingw32/bin/g++.exe']
 compiler=next((str(p) for p in candidates if p.is_file()),None)
if not compiler:raise RuntimeError('Native g++/clang++ required for policy checks')
env=os.environ.copy();env['PATH']=str(Path(compiler).parent)+os.pathsep+env.get('PATH','')
with tempfile.TemporaryDirectory(prefix='v33-policies-') as directory:
 tests=[p for p in sorted((ROOT/'tests').glob('*.cpp')) if not p.name.startswith('native_')]
 for test in tests:
  exe=Path(directory)/(test.stem+('.exe' if os.name=='nt' else ''))
  subprocess.run([compiler,'-std=c++11','-Wall','-Wextra','-Werror','-I'+str(ROOT/'include'),'-I'+str(ROOT/'tests/stubs'),str(test),'-o',str(exe)],env=env,check=True)
  subprocess.run([str(exe)],env=env,check=True);print('PASS '+test.name,flush=True)
 print(str(len(tests))+' native C++ policy executables passed',flush=True)
subprocess.run([sys.executable,'-m','unittest','discover','-s','tests','-p','test_*.py','-v'],cwd=ROOT,check=True)
source=(ROOT/'src/WebDashboardAssets.cpp').read_text()
js=re.search(r'R"js\((.*?)\)js";',source,re.S).group(1)
subprocess.run(['deno','eval','new Function(await new Response(Deno.stdin.readable).text());'],input=js,text=True,cwd=ROOT,check=True)
print('Embedded dashboard JavaScript parse PASS')
