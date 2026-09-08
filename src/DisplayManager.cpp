#include "DisplayManager.h"
#include <SPI.h>
#include "PinConfig.h"
#include "RobotConfig.h"

DisplayManager::DisplayManager()
  : oled_(128, 64, &SPI, PinConfig::OLED_DC, PinConfig::OLED_RESET, PinConfig::OLED_CS),
    initialized_(false),
    page_(0),
    lastRefreshMs_(0),
    lastPageMs_(0) {}

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

void DisplayManager::updateDashboard(const DashboardData& data) {
  if (!initialized_) return;
  const unsigned long now = millis();
  if (lastRefreshMs_ != 0 && now - lastRefreshMs_ < RobotConfig::OLED_REFRESH_INTERVAL_MS) return;
  lastRefreshMs_ = now;
  if (lastPageMs_ == 0 || now - lastPageMs_ >= RobotConfig::OLED_PAGE_INTERVAL_MS) {
    if (lastPageMs_ != 0) page_ = (page_ + 1) % 4;
    lastPageMs_ = now;
  }
  oled_.clearDisplay();
  oled_.setTextSize(1);
  oled_.setTextColor(SH110X_WHITE);
  oled_.setCursor(0, 0);
  if (data.safetyState == "CRITICAL") renderAlert(data);
  else renderPage(data);
  oled_.display();
}

void DisplayManager::renderAlert(const DashboardData& data) {
  oled_.setTextSize(2);
  oled_.println("!!! ALERT !!!");
  oled_.setTextSize(1);
  oled_.println();
  oled_.println(data.alarmReason);
  oled_.println("MOTORS STOPPED");
}

void DisplayManager::renderPage(const DashboardData& data) {
  switch (page_) {
    case 0:
      oled_.println("PIKU 2.1 Rev A");
      oled_.print("MODE: "); oled_.println(data.controlMode);
      oled_.print("SPD: "); oled_.print(data.speedPercent); oled_.println("%");
      oled_.print("MOTOR: "); oled_.println(data.motorState);
      oled_.print("DIST: ");
      if (data.distanceValid) { printValue(data.frontDistance, 1); oled_.println(" cm"); }
      else oled_.println("NO ECHO");
      break;
    case 1:
      oled_.println("ENVIRONMENT");
      oled_.print("TEMP: "); if (data.temperatureValid) { printValue(data.temperature, 1); oled_.println(" C"); } else oled_.println("INVALID");
      oled_.print("HUM : "); if (data.humidityValid) { printValue(data.humidity, 1); oled_.println(" %"); } else oled_.println("INVALID");
      oled_.print("GAS : "); oled_.println(data.gasStatus);
      oled_.print("FLAME: "); oled_.println(data.flameDetected ? "DETECTED" : "CLEAR");
      break;
    case 2:
      oled_.println("IMU / MPU6050");
      if (!data.imuInitialized) { oled_.println("STATUS: ERROR"); break; }
      oled_.print("ROLL : "); printValue(data.roll, 1); oled_.println();
      oled_.print("PITCH: "); printValue(data.pitch, 1); oled_.println();
      oled_.print("MOTION: "); oled_.println(data.motionState);
      break;
    default:
      oled_.println("COMMUNICATION");
      oled_.print("WIFI: AP"); oled_.println(data.staConnected ? "+STA" : "");
      oled_.print("NRF : "); oled_.println(data.radioHardware);
      oled_.print("PEER: "); oled_.println(data.radioPeer);
      oled_.print("TX "); oled_.print(data.radioTxSuccess); oled_.print("/"); oled_.println(data.radioTxAttempts);
      oled_.print("RX "); oled_.println(data.radioRxPackets);
      break;
  }
}

void DisplayManager::printValue(float value, uint8_t decimals) {
  if (isfinite(value)) oled_.print(value, decimals);
  else oled_.print("--");
}
