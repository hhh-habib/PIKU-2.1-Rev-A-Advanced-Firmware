#pragma once

#include <RF24.h>
#include "RadioTypes.h"

class RadioManager {
 public:
  RadioManager();
  bool begin();
  void update(uint8_t alarmMask);
  const RadioSnapshot& snapshot() const;
  const char* hardwareStateName() const;
  const char* peerStateName() const;
  const char* dataRateName() const;
  const char* paLevelName() const;

 private:
  RF24 radio_;
  RadioSnapshot snapshot_;
  uint16_t sequence_ = 0;
  unsigned long lastHeartbeatMs_ = 0;

  void pollReceive();
  void sendHeartbeat(uint8_t alarmMask);
  bool validatePacket(const RadioPacket& packet) const;
};
