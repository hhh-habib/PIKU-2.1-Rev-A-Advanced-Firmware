#include "NavigationTrace.h"
#include "RobotConfig.h"
#include "TraceChecksum.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {
template <size_t N>
void copyField(char (&dest)[N], const String& source) {
  snprintf(dest, N, "%s", source.c_str());
}
template <size_t N>
void copyField(char (&dest)[N], const char* source) {
  snprintf(dest, N, "%s", source);
}
void distanceField(float cm, const char* state, char* output, size_t capacity) {
  if (strcmp(state,"DISTANCE_VALID")==0 && isfinite(cm))
    snprintf(output,capacity,"%.1f",static_cast<double>(cm));
  else if (strcmp(state,"NOT_SAMPLED")==0 ||
           strcmp(state,"CLEAR_NO_ECHO")==0)
    snprintf(output,capacity,"NOT_MEASURED");
  else
    snprintf(output,capacity,"INVALID");
}
}

void NavigationTrace::begin(uint32_t bootId) {
  bootId_=bootId;
  enabled_=false;
  ring_.clear();
}

void NavigationTrace::service(const DashboardData& data) {
  readCommand();
  const unsigned long now=millis();
  if (enabled_) observe(data,now);
  drain(now);
}

void NavigationTrace::readCommand() {
  // At most eight input bytes per service pass; STOP and sensors run first.
  for (uint8_t i=0;i<8 && Serial.available()>0;++i) {
    const char c=static_cast<char>(Serial.read());
    if (c=='\r') continue;
    if (c=='\n') {
      input_[inputLength_]='\0';
      if (strncmp(input_,"TRACE ON",8)==0 &&
          (input_[8]=='\0' || input_[8]==' ')) {
        const char* candidate=input_[8]==' ' ? input_+9 : "";
        const size_t length=strlen(candidate);
        bool valid=length<sizeof(runId_);
        for (size_t j=0;j<length && valid;++j) {
          const unsigned char value=static_cast<unsigned char>(candidate[j]);
          valid=(value>='A' && value<='Z') ||
                (value>='a' && value<='z') ||
                (value>='0' && value<='9') ||
                value=='_' || value=='-';
        }
        if (valid) {
          snprintf(runId_,sizeof(runId_),"%s",length==0?"NOT_PROVIDED":candidate);
          ring_.clear();
          enabled_=true;
          headerPending_=true;
          lineLength_=lineOffset_=0;
          lastRecordMs_=0;
          lastDecision_[0]=lastState_[0]=lastMode_[0]=lastOutcome_[0]='\0';
          lastHazardMask_=lastIrMask_=0;
        }
      } else if (strcmp(input_,"TRACE OFF")==0) {
        enabled_=false;
        headerPending_=false;
        ring_.clear();
        lineLength_=lineOffset_=0;
      }
      inputLength_=0;
    } else if (inputLength_ < sizeof(input_)-1) {
      input_[inputLength_++]=c;
    } else {
      inputLength_=0;
    }
  }
}

void NavigationTrace::observe(const DashboardData& data, unsigned long now) {
  const uint8_t hazardMask=(data.flameCritical?1:0) |
      (data.gasCritical?2:0) |
      (data.temperatureCritical?4:0);
  const uint8_t irMask=(data.leftObstacle?1:0) | (data.rightObstacle?2:0);
  const bool autoMode=data.controlMode=="AUTO";
  const char* currentState=autoMode ?
      data.autoNavigationState.c_str() : "MANUAL";
  const char* outcome=autoMode && data.autoAssistanceRequired ? "TRAP_HOLD" :
      autoMode && data.autoHazardRecoveryState=="RESUME_READY" ?
      "CLEARANCE_REGAINED" :
      data.navigationDecision=="OPERATOR_STOP" ||
      data.navigationDecision=="AUTO_OPERATOR_STOP" ||
      data.navigationDecision=="AUTO_MODE_CHANGE" ||
      data.navigationDecision=="MODE_CHANGE_STOP" ||
      data.navigationDecision=="ENVIRONMENTAL_HAZARD_STOP" ||
      data.navigationDecision=="SAFETY_STOP" ? "ABORTED" : "PENDING";
  const bool transition=strcmp(lastMode_,data.controlMode.c_str())!=0 ||
      strcmp(lastState_,currentState)!=0 ||
      strcmp(lastDecision_,data.navigationDecision.c_str())!=0 ||
      strcmp(lastOutcome_,outcome)!=0 ||
      lastHazardMask_!=hazardMask || lastIrMask_!=irMask;
  if (!transition && lastRecordMs_!=0 &&
      now-lastRecordMs_<SAMPLE_INTERVAL_MS) return;

  Record r;
  r.uptimeMs=now;
  r.elapsedMs=lastRecordMs_==0?0:now-lastRecordMs_;
  r.dropped=ring_.dropped();
  r.irLeft=data.leftObstacle; r.irRight=data.rightObstacle;
  r.motorStop=data.motorState=="STOP";
  r.flame=data.flameCritical; r.gas=(hazardMask&2)!=0;
  r.temp=(hazardMask&4)!=0;
  r.attempts=data.autoV2State=="OFF" ?
      data.autoObstacleRecoveryAttempts : data.autoV2Attempts;
  r.leftCm=autoMode ? data.autoScanLeftDistanceCm : NAN;
  r.frontCm=autoMode ? data.autoScanFrontDistanceCm : NAN;
  r.rightCm=autoMode ? data.autoScanRightDistanceCm : NAN;
  r.relativeTurnDeg=autoMode ? data.autoRelativeTurnDeg : NAN;
  r.targetTurnDeg=autoMode ? data.autoTurnTargetDeg : NAN;
  copyField(r.mode,data.controlMode);
  snprintf(r.state,sizeof(r.state),"%s",currentState);
  copyField(r.decision,data.navigationDecision);
  copyField(r.leftValidity,autoMode ? data.autoScanLeftState.c_str() : "NOT_SAMPLED");
  copyField(r.frontValidity,autoMode ? data.autoScanFrontState.c_str() : "NOT_SAMPLED");
  copyField(r.rightValidity,autoMode ? data.autoScanRightState.c_str() : "NOT_SAMPLED");
  if (autoMode) {
    if (strcmp(r.leftValidity,"DISTANCE_VALID")==0 &&
        !data.autoScanLeftTrusted) snprintf(r.leftValidity,sizeof(r.leftValidity),"STALE");
    if (strcmp(r.frontValidity,"DISTANCE_VALID")==0 &&
        !data.autoScanFrontTrusted) snprintf(r.frontValidity,sizeof(r.frontValidity),"STALE");
    if (strcmp(r.rightValidity,"DISTANCE_VALID")==0 &&
        !data.autoScanRightTrusted) snprintf(r.rightValidity,sizeof(r.rightValidity),"STALE");
  }
  copyField(r.gyroStatus,data.imuHealthState);
  snprintf(r.outcome,sizeof(r.outcome),"%s",outcome);
  snprintf(r.kind,sizeof(r.kind),"%s",transition?"EVENT":"OBSERVATION");
  ring_.push(r);
  snprintf(lastMode_,sizeof(lastMode_),"%s",r.mode);
  snprintf(lastState_,sizeof(lastState_),"%s",r.state);
  snprintf(lastDecision_,sizeof(lastDecision_),"%s",r.decision);
  snprintf(lastOutcome_,sizeof(lastOutcome_),"%s",outcome);
  lastHazardMask_=hazardMask; lastIrMask_=irMask;
  lastRecordMs_=now;
}

void NavigationTrace::prepareLine(const Record& r) {
  char left[20],front[20],right[20],relative[20],target[20],observed[20];
  distanceField(r.leftCm,r.leftValidity,left,sizeof(left));
  distanceField(r.frontCm,r.frontValidity,front,sizeof(front));
  distanceField(r.rightCm,r.rightValidity,right,sizeof(right));
  if (isfinite(r.relativeTurnDeg) && strcmp(r.gyroStatus,"HEALTHY")==0)
    snprintf(relative,sizeof(relative),"%.1f",static_cast<double>(r.relativeTurnDeg));
  else snprintf(relative,sizeof(relative),"INVALID");
  if (strstr(r.decision,"SPIN") || strstr(r.state,"TURN"))
    snprintf(target,sizeof(target),"%.1f",static_cast<double>(r.targetTurnDeg));
  else snprintf(target,sizeof(target),"NOT_MEASURED");
  if (strcmp(target,"NOT_MEASURED")!=0 &&
      strcmp(relative,"INVALID")!=0)
    snprintf(observed,sizeof(observed),"%s",relative);
  else snprintf(observed,sizeof(observed),"NOT_MEASURED");
  const int count=snprintf(line_,sizeof(line_),
    "NAVCSV,1,%s,%08lx,%lu,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%u,%u,%s,%s,%s,%s,%u,%u,%u,%u,%u,%lu,%s,%lu",
    RobotConfig::FIRMWARE_VERSION,
    static_cast<unsigned long>(bootId_),
    static_cast<unsigned long>(r.uptimeMs),
    runId_,r.kind,r.mode,r.state,r.decision,
    left,r.leftValidity,front,r.frontValidity,right,r.rightValidity,
    r.irLeft,r.irRight,r.gyroStatus,relative,target,observed,
    r.attempts,r.motorStop,r.flame,r.gas,r.temp,
    static_cast<unsigned long>(r.elapsedMs),r.outcome,
    static_cast<unsigned long>(r.dropped));
  if (count<0 || static_cast<size_t>(count)+11>=sizeof(line_)) {
    snprintf(line_,sizeof(line_),"NAVCSV,ERROR_FORMAT\n");
  } else {
    const uint32_t checksum=traceChecksum(line_,static_cast<size_t>(count));
    snprintf(line_+count,sizeof(line_)-count,",%08lX\n",
             static_cast<unsigned long>(checksum));
  }
  lineLength_=strlen(line_);
  lineOffset_=0;
}

void NavigationTrace::drain(unsigned long now) {
  if (now-lastDrainMs_<DRAIN_INTERVAL_MS) return;
  lastDrainMs_=now;
  if (lineOffset_>=lineLength_) {
    if (headerPending_) {
      snprintf(line_,sizeof(line_),
        "NAVCSV,schema_version,firmware_id,boot_id,uptime_ms,run_id,kind,mode,nav_state,decision,left_cm,left_validity,front_cm,front_validity,right_cm,right_validity,ir_left,ir_right,gyro_status,gyro_relative_est_deg,intended_turn_deg,observed_turn_est_deg,recovery_attempt,motor_stop,flame,gas,temp,elapsed_ms,outcome,dropped_total,checksum_fnv1a32\n");
      lineLength_=strlen(line_);
      lineOffset_=0;
      headerPending_=false;
    } else {
      Record r;
      if (!ring_.pop(r)) return;
      prepareLine(r);
    }
  }
  const int writable=Serial.availableForWrite();
  if (writable<=0) return;
  size_t chunk=lineLength_-lineOffset_;
  if (chunk>DRAIN_CHUNK_BYTES) chunk=DRAIN_CHUNK_BYTES;
  if (chunk>static_cast<size_t>(writable)) chunk=writable;
  if (chunk==0) return;
  lineOffset_+=Serial.write(
      reinterpret_cast<const uint8_t*>(line_)+lineOffset_,chunk);
}
