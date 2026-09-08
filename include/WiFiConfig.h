#ifndef WIFICONFIG_H
#define WIFICONFIG_H

#include <Arduino.h>

#ifndef PIKU_HOME_WIFI_SSID
#define PIKU_HOME_WIFI_SSID "IUB-IRCPS"
#endif

#ifndef PIKU_HOME_WIFI_PASSWORD
#define PIKU_HOME_WIFI_PASSWORD "piku1234"
#endif

namespace WiFiConfig {
  static constexpr const char* COORDINATOR_SSID = "IUB-IRCPS";
  static constexpr uint8_t COORDINATOR_STA_IP[4] = {192, 168, 10, 22};
  static constexpr uint8_t COORDINATOR_GATEWAY[4] = {192, 168, 10, 1};
  static constexpr uint8_t COORDINATOR_SUBNET[4] = {255, 255, 255, 0};
  static constexpr const char* COORDINATOR_ORIGIN = "http://192.168.10.1";
  static constexpr const char* HOME_WIFI_SSID = PIKU_HOME_WIFI_SSID;
  static constexpr const char* HOME_WIFI_PASSWORD = PIKU_HOME_WIFI_PASSWORD;
}

#endif // WIFICONFIG_H
