"""Execute current NavigationManager with hardware doubles, one isolated collision per process."""
from pathlib import Path
import os,re,shutil,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
def declaration(text,marker):
 a=text.index(marker);b=text.index('{',a);depth=1;c=b+1
 while depth:
  depth+=(text[c]=='{')-(text[c]=='}');c+=1
 if text[c:c+1]==';':c+=1
 return text[a:c]
class StateCollisions(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.directory=tempfile.TemporaryDirectory(prefix='state-collisions-');work=cls.work=Path(cls.directory.name)
  stage4=(ROOT/'include/CoordinatorLossPolicy.h').exists()
  arduino='#pragma once\n#include <cstdint>\n#include <cstddef>\n#include <cmath>\n#include <cstring>\n#include <string>\nusing String=std::string;using std::isfinite;\n#define HIGH 1\n#define LOW 0\n#define RAD_TO_DEG 57.29577951308232f\ninline unsigned long testNow=10000;inline unsigned long millis(){return testNow;}\nstruct Log{template<typename...A>void printf(const char*,A...){}void println(const char*){}};inline Log Serial;\n'
  (work/'Arduino.h').write_text(arduino)
  types='#pragma once\n#include "Arduino.h"\n'
  pairs=[('SensorManager',['enum class UltrasonicResultState','enum class FlameState','struct UltrasonicSample','struct SensorSnapshot']),('ImuManager',['struct ImuSnapshot']),('MotorController',['enum class MotorState']),('WebDashboard',['struct DashboardData'])]
  if stage4:pairs[-1][1].extend(['enum class DriveCommandSource','enum class ManualSensorHeadCommand'])
  for name,markers in pairs:
   text=(ROOT/'include'/f'{name}.h').read_text(encoding='utf-8-sig')
   for marker in markers:types+=declaration(text,marker)+'\n'
  if stage4:
   # Hardware fixture includes RF/control types; retain the production declarations.
   text=(ROOT/'src/main.cpp').read_text()
   for marker in ['enum class ManualSensorHeadState','struct ManualSensorHeadOperation','struct ManualInspectionState']:types+=declaration(text,marker)+'\n'
   types+=declaration((ROOT/'include/RadioManager.h').read_text(),'struct RadioCommandEvent')+'\n'
   radio=(ROOT/'include/RadioTypes.h').read_text().replace('__attribute__((packed))','')
   (work/'RadioTypes.h').write_text('#pragma pack(push,1)\n'+radio+'\n#pragma pack(pop)\n')
   types='#include "RadioTypes.h"\n'+types
  (work/'NativeTypes.h').write_text(types)
  doubles=ROOT/('tests/fixtures/RobotDoubles.h' if stage4 else 'tests/v33_doubles.h')
  shutil.copy(doubles,work/'CollisionDoubles.h')
  header=(ROOT/'include/NavigationManager.h').read_text()
  header=re.sub(r'#include "(?:ImuManager|MotorController|SafetyManager|SensorManager|ServoScanner|WebDashboard)\.h"','',header).replace('\nprivate:\n','\npublic:\n')
  (work/'NavigationUnderTest.h').write_text('#include "CollisionDoubles.h"\n'+header)
  (work/'NavigationUnderTest.cpp').write_text((ROOT/'src/NavigationManager.cpp').read_text().replace('#include "NavigationManager.h"','#include "NavigationUnderTest.h"'))
  cls.exe=work/'collisions.exe'
  vcvars=Path(os.environ.get('PIKU_VCVARS64',str(Path(ROOT.anchor)/'Development/VisualStudio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat')))
  inputs=[getattr(cls,'fixture',ROOT/'tests/native_state_collisions.cpp'),work/'NavigationUnderTest.cpp']
  defines=['/DCPS_COLLISION'] if stage4 else []
  if vcvars.is_file():
   args=['cl','/nologo','/std:c++17','/EHsc','/W4',*defines,'/I'+str(work),'/I'+str(ROOT/'include'),*map(str,inputs),'/Fe:'+str(cls.exe)]
   batch=work/'build.cmd';batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(args)+'\nexit /b %errorlevel%\n');command='cmd /d /s /c ""'+str(batch)+'""'
  else:
   compiler=shutil.which('g++') or shutil.which('clang++')
   if not compiler:raise RuntimeError('Native compiler required')
   command=[compiler,'-std=c++17','-Wall','-Wextra',*(['-DCPS_COLLISION'] if stage4 else []),'-I'+str(work),'-I'+str(ROOT/'include'),*map(str,inputs),'-o',str(cls.exe)]
  built=subprocess.run(command,cwd=work,text=True,capture_output=True,timeout=90)
  if built.returncode:raise RuntimeError(built.stdout+built.stderr)
 @classmethod
 def tearDownClass(cls):cls.directory.cleanup()
 def case(self,name):
  run=subprocess.run([str(self.exe),name],cwd=self.work,text=True,capture_output=True,timeout=20)
  self.assertEqual(run.returncode,0,run.stdout+run.stderr)
 def test_environmental_sonar_context(self):self.case('env_sonar')
 def test_environmental_sonar_fresh_scan(self):self.case('env_resume')
 def test_environmental_ir_keeps_scan_authority(self):self.case('env_ir')
 def test_episode_hazard_requires_environmental_scan(self):self.case('env_episode')
 def test_environmental_scan_counts_active_work(self):self.case('env_watch')
 def test_nested_recovery_hazard_sonar(self):self.case('nested_recovery_hazard_sonar')
 def test_trapped_hazard_preserves_deadline(self):self.case('trapped_hazard')
 def test_watchdog_cooldown_accumulates(self):self.case('watch_cooldown')
 def test_watchdog_sonar_hold_accumulates(self):self.case('watch_pause')
 def test_trapped_pause_preserves_cooldown(self):self.case('trapped_pause')
 def test_auto_stop_mode_rearm(self):self.case('auto_stop')
 def test_sonar_invalidates_pending_turn_completion(self):self.case('sonar_turn')
 def test_critical_beats_forward(self):self.case('critical_forward')
 def test_environmental_repeated_sonar(self):self.case('env_repeated')
 def test_environmental_hazard_restarts_clear_wait(self):self.case('env_new_hazard')
 def test_environmental_mode_exit_cancels(self):self.case('env_mode')
 def test_boot_starts_stopped(self):self.case('boot')
 def test_integration_environmental_fresh_scan(self):
  if not (ROOT/'include/CoordinatorLossPolicy.h').exists():self.skipTest('CPS seam only')
  self.case('env_integration')
 def test_nested_sonar_then_integration(self):
  if not (ROOT/'include/CoordinatorLossPolicy.h').exists():self.skipTest('CPS seam only')
  self.case('nested_sonar_integration')
 def test_nested_integration_then_sonar(self):
  if not (ROOT/'include/CoordinatorLossPolicy.h').exists():self.skipTest('CPS seam only')
  self.case('nested_integration_sonar')
if __name__=='__main__':unittest.main()
