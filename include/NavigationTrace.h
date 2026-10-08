#ifndef NAVIGATION_TRACE_H
#define NAVIGATION_TRACE_H

#include <Arduino.h>
#include "TraceRing.h"
#include "WebDashboard.h"

// Optional, serial-only research trace. Starts OFF on each boot; no flash writes.
class NavigationTrace {
public:
  void begin(uint32_t bootId);
  void service(const DashboardData& data);
private:
  struct Record {
    uint32_t uptimeMs=0, elapsedMs=0, dropped=0;
    uint8_t irLeft=0, irRight=0, motorStop=0, flame=0, gas=0, temp=0;
    uint8_t attempts=0;
    float leftCm=NAN, frontCm=NAN, rightCm=NAN;
    float relativeTurnDeg=NAN, targetTurnDeg=NAN;
    char mode[9]{}, state[26]{}, decision[49]{};
    char leftValidity[20]{}, frontValidity[20]{}, rightValidity[20]{};
    char gyroStatus[24]{}, outcome[24]{};
    char kind[13]{};
  };
  static constexpr unsigned long SAMPLE_INTERVAL_MS = 1000;
  static constexpr unsigned long DRAIN_INTERVAL_MS = 20;
  static constexpr size_t DRAIN_CHUNK_BYTES = 48;
  TraceRing<Record,16> ring_;
  bool enabled_=false, headerPending_=false;
  uint32_t bootId_=0;
  char runId_[17]="NOT_PROVIDED";
  char input_[48]{};
  uint8_t inputLength_=0;
  unsigned long lastRecordMs_=0, lastDrainMs_=0;
  char lastMode_[9]{}, lastState_[26]{}, lastDecision_[49]{};
  char lastOutcome_[24]{};
  uint8_t lastHazardMask_=0, lastIrMask_=0;
  char line_[640]{};
  size_t lineLength_=0, lineOffset_=0;
  void readCommand();
  void observe(const DashboardData& data, unsigned long now);
  void prepareLine(const Record& record);
  void drain(unsigned long now);
};
#endif
