#include "DisplayManager.h"
#include <SPI.h>
#include <cstdio>
#include <cstring>
#include "PinConfig.h"
#include "RobotConfig.h"

namespace {
uint8_t environmentalAlertMask(const DashboardData& data) {
  uint8_t mask = (data.flameCritical ? 1 : 0) | (data.gasCritical ? 2 : 0) |
                 (data.temperatureCritical ? 4 : 0);
  // Accept the existing primary confirmed alarm; distance never becomes an overlay.
  if (data.safetyState == "CRITICAL") {
    if (data.alarmReason == "FLAME") mask |= 1;
    if (data.alarmReason == "GAS_HIGH_RELATIVE") mask |= 2;
    if (data.alarmReason == "TEMP_HIGH") mask |= 4;
  }
  return mask;
}

void formatDisplayLines(const DashboardData& d, uint8_t mask, char (&lines)[8][22]) {
  if (mask) {
    snprintf(lines[0], 22, "ENVIRONMENT ALERT");
    uint8_t row = 2;
    if (mask & 1) snprintf(lines[row++], 22, "FLAME DETECTED");
    if (mask & 2) snprintf(lines[row++], 22, "GAS HIGH (%d)", d.gasValue);
    if (mask & 4) {
      if (d.temperatureValid && isfinite(d.temperature))
        snprintf(lines[row++], 22, "TEMP HIGH: %.1f C", double(d.temperature));
      else snprintf(lines[row++], 22, "TEMP HIGH");
    }
    snprintf(lines[7], 22, "HAZARD ACTIVE");
    return;
  }
  snprintf(lines[0], 22, "PIKU 2.1 Rev A");
  if (d.distanceValid && isfinite(d.frontDistance) && d.frontDistance > 0)
    snprintf(lines[1], 22, "DIST: %.1f cm", double(d.frontDistance));
  else snprintf(lines[1], 22, "DIST: %s", d.ultrasonicState == "CLEAR_NO_ECHO" ? "OPEN" :
    d.ultrasonicState == "SENSOR_FAULT" ? "FAULT" : "--");
  char temperature[12] = "--", humidity[8] = "--";
  if (d.temperatureValid && isfinite(d.temperature) &&
      d.environmentSampleAgeMs <= RobotConfig::DHT22_INTERVAL_MS * 2)
    snprintf(temperature, sizeof(temperature), "%.1f", double(d.temperature));
  if (d.humidityValid && isfinite(d.humidity) &&
      d.environmentSampleAgeMs <= RobotConfig::DHT22_INTERVAL_MS * 2)
    snprintf(humidity, sizeof(humidity), "%.0f", double(d.humidity));
  snprintf(lines[2], 22, "T:%sC H:%s%%", temperature, humidity);
  const char* gas = d.gasStatus == "LOW_RELATIVE" ? "LOW" :
    d.gasStatus == "ELEVATED_RELATIVE" ? "ELEV" :
    d.gasStatus == "HIGH_RELATIVE" ? "HIGH" : "--";
  if (d.mq2Warmup) snprintf(lines[3], 22, "GAS: WARMING (%d)", d.gasValue);
  else if (gas[0] != '-' && d.mq2SampleAgeMs <= RobotConfig::MQ2_INTERVAL_MS * 2)
    snprintf(lines[3], 22, "GAS: %s (%d)", gas, d.gasValue);
  else snprintf(lines[3], 22, "GAS: UNAVAILABLE");
  snprintf(lines[4], 22, "FLAME: %s", !d.flameSampleValid ? "INVALID" :
    d.flameDetected ? "DETECTED" : d.flameState == "CLEAR" ? "CLEAR" : "POSSIBLE");
  const bool peerOnline = d.radioEnabled && d.radioInitialized && d.radioChipDetected &&
                          d.radioPeer == "ONLINE";
  snprintf(lines[5], 22, "RADIO: %s", peerOnline ? "CONNECTED" : "DISCONNECTED");
  snprintf(lines[6], 22, "STA: %s", d.staConnected && d.staIp != "0.0.0.0" &&
           d.staIp.length() > 0 ? d.staIp.c_str() : "DISCONNECTED");
}
}

DisplayManager::DisplayManager()
  : oled_(128, 64, &SPI, PinConfig::OLED_DC, PinConfig::OLED_RESET, PinConfig::OLED_CS),
    initialized_(false),
    lastRefreshMs_(0) {}

bool DisplayManager::begin() {
  initialized_ = oled_.begin(0, true);
  if (!initialized_) {
    Serial.println("[Display] OLED ERROR; robot services will continue");
    return false;
  }
  oled_.setRotation(2);
  oled_.clearDisplay();
  oled_.setTextColor(SH110X_WHITE);
  oled_.setTextSize(1);
  oled_.setCursor(0, 0);
  oled_.println("PIKU 2.1 Rev A");
  oled_.println("STAGE-2 STARTING");
  oled_.display();
  Serial.println("[Display] SH1106 READY (shared hardware SPI)");
  return true;
}

bool DisplayManager::initialized() const { return initialized_; }

bool DisplayManager::alertUpdatePending(const DashboardData& data) const {
  return initialized_ && environmentalAlertMask(data) != lastAlertMask_;
}

void DisplayManager::updateDashboard(const DashboardData& data) {
  if (!initialized_) return;
  const unsigned long now = millis();
  const bool urgent = alertUpdatePending(data);
  if (!urgent && hasFrame_ && now - lastRefreshMs_ < 1000) return;
  lastRefreshMs_ = now;
  char next[8][22]{};
  const uint8_t mask = environmentalAlertMask(data);
  formatDisplayLines(data, mask, next);
  if (hasFrame_ && memcmp(lines_, next, sizeof(lines_)) == 0) return;
  memcpy(lines_, next, sizeof(lines_));
  oled_.clearDisplay();
  oled_.setTextSize(1);
  oled_.setTextColor(SH110X_WHITE);
  oled_.setCursor(0, 0);
  if (mask) renderAlert(data);
  else renderPage(data);
  oled_.display();
  hasFrame_ = true;
  lastAlertMask_ = mask;
}

void DisplayManager::renderAlert(const DashboardData&) {
  for (const auto& line : lines_) oled_.println(line);
}

void DisplayManager::renderPage(const DashboardData&) {
  for (const auto& line : lines_) oled_.println(line);
}

void DisplayManager::printValue(float value, uint8_t decimals) {
  if (isfinite(value)) oled_.print(value, decimals);
  else oled_.print("--");
}
