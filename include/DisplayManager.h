#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include <Adafruit_SH110X.h>
#include "WebDashboard.h"

class DisplayManager {
public:
  DisplayManager();
  bool begin();
  void updateDashboard(const DashboardData& data);
  bool alertUpdatePending(const DashboardData& data) const;
  bool initialized() const;

private:
  Adafruit_SH1106G oled_;
  bool initialized_;
  unsigned long lastRefreshMs_;
  bool hasFrame_ = false;
  uint8_t lastAlertMask_ = 0;
  char lines_[8][22]{}; // 21 text columns at 6px; fixed storage, no rotating pages.

  void renderAlert(const DashboardData& data);
  void renderPage(const DashboardData& data);
  void printValue(float value, uint8_t decimals);
};

#endif // DISPLAYMANAGER_H
