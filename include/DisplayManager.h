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
  bool initialized() const;

private:
  Adafruit_SH1106G oled_;
  bool initialized_;
  uint8_t page_;
  unsigned long lastRefreshMs_;
  unsigned long lastPageMs_;

  void renderAlert(const DashboardData& data);
  void renderPage(const DashboardData& data);
  void printValue(float value, uint8_t decimals);
};

#endif // DISPLAYMANAGER_H
