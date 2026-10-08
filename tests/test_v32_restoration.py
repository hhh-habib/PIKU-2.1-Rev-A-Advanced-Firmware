"""Chronological comparison of actual read-only V3.2 and restored production source."""
from pathlib import Path
import json,os,re,shutil,subprocess,tempfile,unittest,hashlib
from test_state_collisions import declaration
ROOT=Path(__file__).resolve().parents[1]
REFERENCE=Path(r'C:\Users\User\Downloads\PIKU-2.1-Rev-A-earlier github code')
PARITY=['normal','front','left','right','both','clear_single','no_path','history','short','boxed','side4','side6','side7','side_above','increment','front_after','ir_turn','pulse_limit','exhaustion','retry','timeout','exit','open_exit','open','invalid','stale','choice_left','choice_right','tie','single_preference','side20','side_update','normal_fallback','normal_deadline']
PRESERVED=['stop_reverse','stop_turn','stop_recovery','mode','hazard','critical','environment_sonar','environment_ir','safety_matrix']
class Restoration(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.directory=tempfile.TemporaryDirectory(prefix='v32-restoration-',dir=ROOT/'.pio');cls.work=Path(cls.directory.name);cls.executables={};cls.summaries={}
  for label,source in [('reference',REFERENCE),('restored',ROOT)]:
   w=cls.work/label;w.mkdir()
   (w/'Arduino.h').write_text('#pragma once\n#include <cstdint>\n#include <cstddef>\n#include <cmath>\n#include <cstring>\n#include <string>\nusing String=std::string;using std::isfinite;\n#define HIGH 1\n#define LOW 0\n#define RAD_TO_DEG 57.29577951308232f\ninline unsigned long testNow=10000;inline unsigned long millis(){return testNow;}\nstruct Log{template<class...A>void printf(const char*,A...){}void println(const char*){}};inline Log Serial;\n')
   types='#pragma once\n#include "Arduino.h"\n'
   for name,markers in [('SensorManager',['enum class UltrasonicResultState','enum class FlameState','struct UltrasonicSample','struct SensorSnapshot']),('ImuManager',['struct ImuSnapshot']),('MotorController',['enum class MotorState']),('WebDashboard',['struct DashboardData'])]:
    text=(ROOT/'include'/f'{name}.h').read_text(encoding='utf-8-sig')
    for marker in markers:types+=declaration(text,marker)+'\n'
   (w/'NativeTypes.h').write_text(types);shutil.copy(ROOT/'tests/v33_doubles.h',w/'CollisionDoubles.h')
   h=(source/'include/NavigationManager.h').read_text();h=re.sub(r'#include "(?:ImuManager|MotorController|SafetyManager|SensorManager|ServoScanner|WebDashboard)\.h"','',h).replace('\nprivate:\n','\npublic:\n')
   (w/'NavigationUnderTest.h').write_text('#include "CollisionDoubles.h"\n'+h)
   (w/'NavigationUnderTest.cpp').write_text((source/'src/NavigationManager.cpp').read_text().replace('#include "NavigationManager.h"','#include "NavigationUnderTest.h"'))
   exe=w/'navigation.exe';vcvars=Path(ROOT.anchor)/'Development/VisualStudio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
   args=['cl','/nologo','/std:c++17','/EHsc','/W4',*(['/DV32_REFERENCE'] if label=='reference' else []),'/I'+str(w),'/I'+str(source/'include'),str(ROOT/'tests/native_v32_restoration.cpp'),str(w/'NavigationUnderTest.cpp'),'/Fe:'+str(exe)]
   if vcvars.is_file():
    batch=w/'build.cmd';batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(args)+'\nexit /b %errorlevel%\n');command='cmd /d /s /c ""'+str(batch)+'""'
   else:
    compiler=shutil.which('g++') or shutil.which('clang++');assert compiler
    command=[compiler,'-std=c++17','-Wall','-Wextra',*(['-DV32_REFERENCE'] if label=='reference' else []),'-I'+str(w),'-I'+str(source/'include'),str(ROOT/'tests/native_v32_restoration.cpp'),str(w/'NavigationUnderTest.cpp'),'-o',str(exe)]
   built=subprocess.run(command,cwd=w,capture_output=True,text=True,timeout=90)
   if built.returncode:raise RuntimeError(built.stdout+built.stderr)
   cls.executables[label]=exe
 @classmethod
 def tearDownClass(cls):
  (ROOT/'docs/V32_RESTORATION_TRACE_SUMMARY.json').write_text(json.dumps(cls.summaries,indent=2)+'\n',encoding='utf-8')
  cls.directory.cleanup()
 def run_case(self,label,case):
  result=subprocess.run([str(self.executables[label]),case],cwd=self.work,text=True,capture_output=True,timeout=20)
  self.assertEqual(result.returncode,0,label+' '+case+'\n'+result.stdout[-2500:]+result.stderr);return result.stdout
 def compare(self,case):
  reference=self.run_case('reference',case);restored=self.run_case('restored',case)
  self.assertEqual(restored,reference,'state/motor/sensor/deadline trace mismatch: '+case)
  rows=[line.split(',') for line in restored.splitlines()];last=rows[-1]
  self.summaries[case]={'trace_match':True,'control_passes':len(rows),'forward_commands':int(last[3]),'reverse_commands':int(last[4]),'spin_commands':int(last[5]),'sensor_requests':int(last[6]),'servo_commands':int(last[7]),'trace_sha256':hashlib.sha256(restored.encode()).hexdigest()}
 def preserved(self,case):self.run_case('restored',case)
for case in PARITY:setattr(Restoration,'test_parity_'+case,lambda self,case=case:self.compare(case))
for case in PRESERVED:setattr(Restoration,'test_preserved_'+case,lambda self,case=case:self.preserved(case))
if __name__=='__main__':unittest.main()
