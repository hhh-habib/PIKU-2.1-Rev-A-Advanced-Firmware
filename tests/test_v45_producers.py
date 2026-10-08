"""Execute current sensor producers, evidence safety and navigation with pin/DHT/IMU/motor doubles."""
from pathlib import Path
import os, re, shutil, subprocess, tempfile, unittest
import test_state_collisions as native
ROOT=Path(__file__).resolve().parents[1]

class Producers(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.directory=tempfile.TemporaryDirectory(prefix='v45-producers-');cls.work=w=Path(cls.directory.name)
  (w/'Arduino.h').write_text('''#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <string>
using String=std::string;using std::isfinite;
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define ADC_11db 0
#define CHANGE 0
#define IRAM_ATTR
#define RAD_TO_DEG 57.29577951308232f
inline unsigned long testNow=10000,testUs=0;inline int analogValue=100,digitalValue=HIGH;
inline unsigned long millis(){return testNow;}inline unsigned long micros(){return testUs;}
inline void pinMode(int,int){}inline void digitalWrite(int,int){}inline int digitalRead(int){return digitalValue;}
inline int analogRead(int){return analogValue;}inline void analogReadResolution(int){}inline void analogSetPinAttenuation(int,int){}
inline int digitalPinToInterrupt(int p){return p;}inline void attachInterrupt(int,void(*)(),int){}
struct Log{template<class...T>void printf(const char*,T...){}void println(const char*){}};inline Log Serial;
''')
  (w/'DHT.h').write_text('''#pragma once
#include "Arduino.h"
#define DHT22 22
inline float testTemperature=20,testHumidity=50;inline unsigned dhtReads=0;
struct DHT{DHT(int,int){}void begin(){}float readHumidity(){++dhtReads;return testHumidity;}float readTemperature(){return testTemperature;}};
''')
  header=(ROOT/'include/SensorManager.h').read_text().replace('\nprivate:\n','\npublic:\n')
  (w/'SensorManager.h').write_text(header)
  (w/'SensorManager.cpp').write_text((ROOT/'src/SensorManager.cpp').read_text())
  for suffix in ('h','cpp'):
   (w/('EvidenceSafetyManager.'+suffix)).write_text((ROOT/('include' if suffix=='h' else 'src')/('SafetyManager.'+suffix)).read_text().replace('SafetyManager','EvidenceSafetyManager'))
  stage4=(ROOT/'include/CoordinatorLossPolicy.h').exists()
  types='#pragma once\n#include "SensorManager.h"\n'
  for name,markers in [('ImuManager',['struct ImuSnapshot']),('MotorController',['enum class MotorState']),('WebDashboard',['struct DashboardData']+(['enum class DriveCommandSource','enum class ManualSensorHeadCommand'] if stage4 else []))]:
   for marker in markers:types+=native.declaration((ROOT/'include'/f'{name}.h').read_text(),marker)+'\n'
  if stage4:
   for marker in ['enum class ManualSensorHeadState','struct ManualSensorHeadOperation','struct ManualInspectionState']:types+=native.declaration((ROOT/'src/main.cpp').read_text(),marker)+'\n'
   radio=(ROOT/'include/RadioTypes.h').read_text().replace('__attribute__((packed))','')
   (w/'RadioTypes.h').write_text('#pragma pack(push,1)\n'+radio+'\n#pragma pack(pop)\n')
   types='#include "RadioTypes.h"\n'+types
  (w/'NativeTypes.h').write_text(types)
  doubles=(ROOT/('tests/fixtures/RobotDoubles.h' if stage4 else 'tests/v33_doubles.h')).read_text()
  a=doubles.index('class SensorManager');b=doubles.index('class ImuManager',a)
  doubles=doubles[:a]+doubles[b:]
  (w/'CollisionDoubles.h').write_text(doubles)
  nav=(ROOT/'include/NavigationManager.h').read_text()
  nav=re.sub(r'#include "(?:ImuManager|MotorController|SafetyManager|SensorManager|ServoScanner|WebDashboard)\.h"','',nav).replace('\nprivate:\n','\npublic:\n')
  (w/'NavigationUnderTest.h').write_text('#include "CollisionDoubles.h"\n'+nav)
  (w/'NavigationUnderTest.cpp').write_text((ROOT/'src/NavigationManager.cpp').read_text().replace('"NavigationManager.h"','"NavigationUnderTest.h"'))
  exe=cls.exe=w/'producers.exe';inputs=[ROOT/'tests/native_v45_producers.cpp',w/'SensorManager.cpp',w/'EvidenceSafetyManager.cpp',w/'NavigationUnderTest.cpp']
  vcvars=Path(ROOT.anchor)/'Development/VisualStudio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
  args=['cl','/nologo','/std:c++17','/EHsc','/W4',*(['/DCPS_COLLISION'] if stage4 else []),'/I'+str(w),'/I'+str(ROOT/'include'),*map(str,inputs),'/Fe:'+str(exe)]
  if vcvars.is_file():
   batch=w/'build.cmd';batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(args)+'\nexit /b %errorlevel%\n');command='cmd /d /s /c ""'+str(batch)+'""'
  else:
   compiler=shutil.which('g++') or shutil.which('clang++')
   if not compiler:raise RuntimeError('Native compiler required')
   command=[compiler,'-std=c++17',*(['-DCPS_COLLISION'] if stage4 else []),'-I'+str(w),'-I'+str(ROOT/'include'),*map(str,inputs),'-o',str(exe)]
  run=subprocess.run(command,cwd=w,text=True,capture_output=True,timeout=90)
  if run.returncode:raise RuntimeError(run.stdout+run.stderr)
 @classmethod
 def tearDownClass(cls):cls.directory.cleanup()
 def case(self,name):
  run=subprocess.run([str(self.exe),name],cwd=self.work,text=True,capture_output=True,timeout=20)
  self.assertEqual(run.returncode,0,run.stdout+run.stderr)
 def test_aud02_failed_dht_gas_cannot_clear_or_renew(self):self.case('freshness')
 def test_aud02_cadence_and_startup_warmup_scoped_override(self):self.case('cadence')
 def test_n20_actual_request_producer_rollover_old_facing_and_time(self):self.case('request')
 def test_aud02_disabled_sensors_have_no_unknown_evidence_hold(self):
  # Execute the real SafetyManager with only numeric/boolean configuration fixtures.
  # Production capability settings remain unchanged; no network/credential constants are copied.
  source=(ROOT/'src/SafetyManager.cpp').read_text(encoding='utf-8')
  sensor=(ROOT/'include/SensorManager.h').read_text(encoding='utf-8')
  config=(ROOT/'include/RobotConfig.h').read_text(encoding='utf-8')
  names=set(re.findall(r'RobotConfig::(\w+)',source+sensor))
  declarations=[]
  for name in sorted(names):
   match=re.search(r'static constexpr ([\w ]+) '+name+r'\s*=\s*([^;]+);',config)
   self.assertIsNotNone(match,name)
   value='false' if name in ('HAS_DHT22','HAS_MQ2','HAS_FLAME_SENSOR') else match.group(2)
   self.assertRegex(value,r'^[0-9.a-zA-Z_ +*/-]+$')
   declarations.append('static constexpr '+match.group(1)+' '+name+'='+value+';')
  (self.work/'RobotConfig.h').write_text('#pragma once\nnamespace RobotConfig {'+'\n'.join(declarations)+'}\n',encoding='utf-8')
  main=self.work/'disabled.cpp'
  main.write_text('#include "EvidenceSafetyManager.h"\n#include <cassert>\nint main(){testNow=100000;SensorSnapshot s;s.environmentStartedMs=0;s.mq2Warmup=false;s.temperatureValid=s.gasValid=false;EvidenceSafetyManager safety;safety.update(s);assert(!safety.monitoringUnavailable()&&!safety.blocksMotion()&&!safety.isCritical());}\n',encoding='utf-8')
  exe=self.work/'disabled.exe';inputs=[main,self.work/'EvidenceSafetyManager.cpp']
  vcvars=Path(ROOT.anchor)/'Development/VisualStudio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
  if vcvars.is_file():
   args=['cl','/nologo','/std:c++17','/EHsc','/I'+str(self.work),'/I'+str(ROOT/'include'),*map(str,inputs),'/Fe:'+str(exe)]
   batch=self.work/'disabled.cmd';batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(args)+'\nexit /b %errorlevel%\n')
   command='cmd /d /s /c ""'+str(batch)+'""'
  else:
   compiler=shutil.which('g++') or shutil.which('clang++')
   if not compiler:raise RuntimeError('Native compiler required')
   command=[compiler,'-std=c++17','-I'+str(self.work),'-I'+str(ROOT/'include'),*map(str,inputs),'-o',str(exe)]
  built=subprocess.run(command,cwd=self.work,text=True,capture_output=True,timeout=90)
  self.assertEqual(built.returncode,0,built.stdout+built.stderr)
  run=subprocess.run([str(exe)],cwd=self.work,text=True,capture_output=True,timeout=20)
  self.assertEqual(run.returncode,0,run.stdout+run.stderr)
