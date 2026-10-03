from pathlib import Path
import os,re,shutil,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
def declaration(text,marker):
 a=text.index(marker);b=text.index('{',a);depth=1;c=b+1
 while depth:
  depth+=(text[c]=='{')-(text[c]=='}');c+=1
 if text[c:c+1]==';':c+=1
 return text[a:c]
class V33ProductionTests(unittest.TestCase):
 def test_production_navigation_interruption_budgets_and_resume(self):
  with tempfile.TemporaryDirectory(prefix='v33-native-') as directory:
   work=Path(directory)
   (work/'Arduino.h').write_text('#pragma once\n#include <cstdint>\n#include <cstddef>\n#include <cmath>\n#include <cstring>\n#include <string>\nusing String=std::string; using std::isfinite;\n#define HIGH 1\n#define LOW 0\n#define RAD_TO_DEG 57.29577951308232f\ninline unsigned long testNow=10000; inline unsigned long millis(){return testNow;}\nstruct NativeSerial {template<typename... A> void printf(const char*,A...){} void println(const char*){}};inline NativeSerial Serial;\n')
   types='#pragma once\n#include "Arduino.h"\n'
   for path,markers in [('include/SensorManager.h',['enum class UltrasonicResultState','enum class FlameState','struct UltrasonicSample','struct SensorSnapshot']),('include/ImuManager.h',['struct ImuSnapshot']),('include/MotorController.h',['enum class MotorState']),('include/WebDashboard.h',['struct DashboardData'])]:
    text=(ROOT/path).read_text(encoding='utf-8-sig')
    for marker in markers:types+=declaration(text,marker)+'\n'
   (work/'NativeTypes.h').write_text(types)
   shutil.copy(ROOT/'tests/v33_doubles.h',work)
   header=(ROOT/'include/NavigationManager.h').read_text()
   header=re.sub(r'#include "(?:ImuManager|MotorController|SafetyManager|SensorManager|ServoScanner|WebDashboard)\.h"','',header).replace('\nprivate:\n','\npublic:\n')
   (work/'NavigationUnderTest.h').write_text('#include "v33_doubles.h"\n'+header)
   (work/'NavigationUnderTest.cpp').write_text((ROOT/'src/NavigationManager.cpp').read_text().replace('#include "NavigationManager.h"','#include "NavigationUnderTest.h"'))
   vcvars=Path(os.environ.get('PIKU_VCVARS64',str(Path(ROOT.anchor)/'Development/VisualStudio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat')))
   compiler=shutil.which('g++') or shutil.which('clang++')
   exe=work/('navigation.exe' if os.name=='nt' else 'navigation')
   includes=[work,ROOT/'include'];inputs=[ROOT/'tests/native_navigation_v33.cpp',work/'NavigationUnderTest.cpp']
   if vcvars.is_file():
    command=['cl','/nologo','/std:c++17','/EHsc','/W4',*['/I'+str(p) for p in includes],*map(str,inputs),'/Fe:'+str(exe)]
    batch=work/'build.cmd';batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(command)+'\nexit /b %errorlevel%\n');command='cmd /d /s /c ""'+str(batch)+'""'
   else:
    self.assertIsNotNone(compiler,'Native compiler required');command=[compiler,'-std=c++17','-Wall','-Wextra',*['-I'+str(p) for p in includes],*map(str,inputs),'-o',str(exe)]
   build=subprocess.run(command,cwd=work,text=True,capture_output=True);self.assertEqual(build.returncode,0,build.stdout+build.stderr)
   run=subprocess.run([str(exe)],cwd=work,text=True,capture_output=True);self.assertEqual(run.returncode,0,run.stdout+run.stderr)
if __name__=='__main__':unittest.main()
