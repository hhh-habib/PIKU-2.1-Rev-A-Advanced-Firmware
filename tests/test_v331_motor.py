"""Execute current production MotorController with only Arduino I/O doubled."""
from pathlib import Path
import os,shutil,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class BackoffMotorTests(unittest.TestCase):
 def test_calibrated_cap_refresh_selected_preference_and_normal_paths(self):
  with tempfile.TemporaryDirectory(prefix='v331-motor-') as directory:
   work=Path(directory)
   (work/'Arduino.h').write_text('#pragma once\n#include <cstdint>\n#include <algorithm>\nusing std::min;\ntemplate<typename T> T constrain(T v,T lo,T hi){return v<lo?lo:v>hi?hi:v;}\n#define OUTPUT 1\n#define LOW 0\n#define HIGH 1\nvoid pinMode(int,int);void digitalWrite(int,int);void ledcSetup(int,int,int);void ledcAttachPin(int,int);void ledcWrite(int,uint32_t);void delay(unsigned long);\nstruct NativeSerial{template<typename T>void print(T){} template<typename T>void println(T){}};extern NativeSerial Serial;\n',encoding='utf8')
   exe=work/'motor.exe';includes=[work,ROOT/'include'];inputs=[ROOT/'tests/native_motor_backoff.cpp',ROOT/'src/MotorController.cpp']
   vcvars=Path(os.environ.get('PIKU_VCVARS64',str(Path(ROOT.anchor)/'Development/VisualStudio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat')))
   if vcvars.is_file():
    command=['cl','/nologo','/std:c++17','/EHsc','/W4',*['/I'+str(p) for p in includes],*map(str,inputs),'/Fe:'+str(exe)]
    batch=work/'build.cmd';batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline(command)+'\nexit /b %errorlevel%\n');command='cmd /d /s /c ""'+str(batch)+'""'
   else:
    compiler=shutil.which('g++') or shutil.which('clang++');self.assertIsNotNone(compiler);command=[compiler,'-std=c++17',*['-I'+str(p) for p in includes],*map(str,inputs),'-o',str(exe)]
   built=subprocess.run(command,cwd=work,text=True,capture_output=True);self.assertEqual(built.returncode,0,built.stdout+built.stderr)
   run=subprocess.run([str(exe)],cwd=work,text=True,capture_output=True);self.assertEqual(run.returncode,0,run.stdout+run.stderr)
