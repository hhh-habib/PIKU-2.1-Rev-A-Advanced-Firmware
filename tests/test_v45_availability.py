"""Owner correction: actual SensorManager/SafetyManager/NavigationManager, hardware doubled."""
from pathlib import Path
import subprocess, shutil, unittest
import test_v45_producers as producers
import test_state_collisions as native
ROOT=Path(__file__).resolve().parents[1]

class Availability(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  producers.Producers.setUpClass();cls.work=w=producers.Producers.work
  doubles=(w/'CollisionDoubles.h').read_text(encoding='utf-8')
  doubles=doubles.replace(native.declaration(doubles,'class SafetyManager'),'#include "EvidenceSafetyManager.h"\nusing SafetyManager=EvidenceSafetyManager;')
  (w/'CollisionDoubles.h').write_text(doubles,encoding='utf-8')
  cls.exe=w/'availability.exe';stage4=(ROOT/'include/CoordinatorLossPolicy.h').exists()
  (w/'AvailabilityDiagnostics.inc').write_text(native.declaration((ROOT/'src/main.cpp').read_text(encoding='utf-8'),'String getGasStatus('),encoding='utf-8')
  if stage4:
   (w/'MotorController.h').write_text('#pragma once\n',encoding='utf-8')
   (w/'RobotRadioStatus.h').write_text((ROOT/'include/RobotRadioStatus.h').read_text(encoding='utf-8'),encoding='utf-8')
   status=(ROOT/'include/RadioStatus.h').read_text(encoding='utf-8').replace('struct __attribute__((packed))','\n#pragma pack(push,1)\nstruct').replace('static_assert(sizeof(RadioStatusPacket)','\n#pragma pack(pop)\nstatic_assert(sizeof(RadioStatusPacket)',1)
   (w/'RadioStatus.h').write_text(status,encoding='utf-8')
  inputs=[ROOT/'tests/native_v45_availability.cpp',w/'SensorManager.cpp',w/'EvidenceSafetyManager.cpp',w/'NavigationUnderTest.cpp']
  vcvars=Path(ROOT.anchor)/'Development/VisualStudio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
  if vcvars.is_file():
   args=['cl','/nologo','/std:c++17','/EHsc',*(['/DCPS_COLLISION'] if stage4 else []),'/I'+str(w),'/I'+str(ROOT/'include'),*map(str,inputs),'/Fe:'+str(cls.exe)]
   batch=w/'availability.cmd';batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(args)+'\nexit /b %errorlevel%\n');command='cmd /d /s /c ""'+str(batch)+'""'
  else:
   compiler=shutil.which('g++') or shutil.which('clang++')
   if not compiler:raise RuntimeError('Native compiler required')
   command=[compiler,'-std=c++17',*(['-DCPS_COLLISION'] if stage4 else []),'-I'+str(w),'-I'+str(ROOT/'include'),*map(str,inputs),'-o',str(cls.exe)]
  built=subprocess.run(command,cwd=w,text=True,capture_output=True,timeout=90)
  if built.returncode:raise RuntimeError(built.stdout+built.stderr)
 @classmethod
 def tearDownClass(cls):producers.Producers.tearDownClass()
 def case(self,name):
  run=subprocess.run([str(self.exe),name],cwd=self.work,text=True,capture_output=True,timeout=30)
  self.assertEqual(run.returncode,0,run.stdout+run.stderr)
 def test_never_valid_stale_invalid_interrupted_prolonged_each_sensor_all_phases(self):self.case('phases')
 def test_success_and_failure_timestamps_are_independent(self):self.case('independent')
 def test_confirmed_hazards_survive_failure_and_require_fresh_clear(self):self.case('hazards')
 def test_unavailable_monitoring_does_not_defeat_stop_sonar_imu_or_authority(self):self.case('gates')
 def test_unavailable_monitoring_preserves_seven_turn_30s_and_ir_cancel(self):self.case('boundaries')
 def test_truthful_unavailable_diagnostics_coexist_with_confirmed_alarm(self):self.case('diagnostics')
