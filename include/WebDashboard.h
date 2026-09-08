#ifndef WEBDASHBOARD_H
#define WEBDASHBOARD_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>

struct DashboardData {
  String product = "PIKU 2.1 Rev A";
  String firmware = "2.1.0-revA-stage2";
  String stage = "STAGE-2";
  String stage2Status = "ACTIVE";
  unsigned long uptimeMs = 0;
  float temperature = NAN;
  bool temperatureValid = false;
  float humidity = NAN;
  bool humidityValid = false;
  unsigned long environmentSampleAgeMs = 0;
  unsigned long dhtErrorCount = 0;
  int gasValue = 0;
  int gasFilteredValue = 0;
  float frontDistance = 0.0f;
  bool distanceValid = false;
  String ultrasonicState = "STALE";
  unsigned long distanceSampleAgeMs = 0;
  unsigned long distanceSampleSequence = 0;
  unsigned long ultrasonicInvalidCount = 0;
  unsigned long ultrasonicFaultCount = 0;
  int servoAngle = 90;
  String scanOrientation = "FRONT";
  bool manualInspectionObstacle = false;
  String manualInspectionOrientation = "NONE";
  bool mq2Warmup = true;
  unsigned long mq2SampleAgeMs = 0;
  unsigned long mq2WarmupAgeMs = 0;
  bool flameDetected = false;
  String flameState = "CLEAR";
  int flameRaw = 0;
  int flameFiltered = 0;
  int flameBaseline = 0;
  int flameDelta = 0;
  bool flameSampleValid = false;
  bool flameBaselineValid = false;
  bool flameClassificationValid = false;
  bool flameThresholdsConfigured = false;
  unsigned long flameSampleAgeMs = 0;
  int leftIrRaw = HIGH;
  int rightIrRaw = HIGH;
  bool leftObstacle = false;
  bool rightObstacle = false;
  bool irObstacleDetected = false;
  bool nearObstacleHazard = false;
  String nearObstacleSource = "NONE";
  bool ultrasonicWarning = false;
  String motorState = "STOP";
  int speedPercent = 60;
  String obstacleDecision = "CLEAR";
  String navigationDecision = "IDLE";
  String gasStatus = "DISABLED";
  String safetyState = "SAFE";
  String alarmReason = "NONE";
  String alarmSoundState = "NOT_INSTALLED";
  String buzzerState = "NOT_INSTALLED";
  String controlMode = "AUTO";
  String turnMode = "PIVOT";
  String softApIp = "0.0.0.0";
  String localHostname = "piku.local";
  bool mdnsActive = false;
  bool staConfigured = false;
  String staSsid = "";
  String staStatus = "NOT_CONFIGURED";
  bool staConnected = false;
  String staIp = "0.0.0.0";
  int staRssi = 0;
  int staSignalQuality = 0;

  bool imuInitialized = false;
  bool imuHealthy = false;
  String imuHealthState = "NOT_INITIALIZED";
  unsigned long imuErrorCount = 0;
  unsigned long imuConsecutiveFailures = 0;
  float accelX = NAN;
  float accelY = NAN;
  float accelZ = NAN;
  float gyroX = NAN;
  float gyroY = NAN;
  float gyroZ = NAN;
  float roll = NAN;
  float pitch = NAN;
  float imuTemperature = NAN;
  String motionState = "ERROR";
  unsigned long imuSampleAgeMs = 0;

  bool radioInitialized = false;
  bool radioChipDetected = false;
  bool radioListening = false;
  int radioChannel = 76;
  String radioDataRate = "250KBPS";
  String radioPaLevel = "LOW";
  unsigned long radioTxAttempts = 0;
  unsigned long radioTxSuccess = 0;
  unsigned long radioTxFailure = 0;
  unsigned long radioRxPackets = 0;
  String radioHardware = "ERROR";
  String radioPeer = "UNSEEN";
  unsigned long radioLastTxAgeMs = 0;
  unsigned long radioLastRxAgeMs = 0;
  bool radioHasTx = false;
  bool radioHasRx = false;

  bool displayInitialized = false;
};

enum class ManualSensorHeadCommand {
  None,
  Left,
  Front,
  Right
};

class WebDashboard {
public:
  WebDashboard();
  void begin(const char* ssid, const char* password, const char* staSsid = "", const char* staPassword = "");
  void loop();
  void setData(const DashboardData& data);
  void setNavigationDecision(const String& decision);
  void setControlMode(const String& mode);
  void setTurnMode(const String& mode);
  void setPendingCommand(const String& command);
  const String& getControlMode() const;
  const String& getTurnMode() const;
  const String& getPendingCommand() const;
  ManualSensorHeadCommand consumeManualSensorHeadCommand();
  bool consumeSpeedPercent(int& speedPercent);
  bool isAlarmEnabled() const;
  bool isAlarmMuted() const;
  bool consumeAlarmTestRequest();
  unsigned long modeCommandCounter() const;
  unsigned long lastCommandReceivedMs() const;
  IPAddress localIP() const;

private:
  WebServer _server;
  DashboardData _data;
  bool _initialized;
  Preferences _preferences;
  enum class StaState {
    NotConfigured,
    Connecting,
    Connected,
    AuthFailed,
    NoAp,
    Disconnected,
    Scanning
  };
  StaState _staState;
  bool _staConfigured;
  bool _staConnectStarted;
  bool _scanStarted;
  bool _lastStaConnected;
  bool _mdnsStarted;
  String _staSsid;
  String _staPassword;
  unsigned long _lastStaAttemptMs;
  bool _alarmEnabled;
  bool _alarmMuted;
  bool _alarmTestRequested;
  unsigned long _modeCommandCounter;
  unsigned long _lastCommandReceivedMs;
  unsigned long _lastCommandSequence;
  unsigned long _lastWiFiUpdateMs;
  unsigned long _lastWiFiTelemetryMs;
  String _pendingCommand;
  int _pendingSpeedPercent;
  ManualSensorHeadCommand _pendingManualSensorHeadCommand;
  struct WiFiTelemetryCache {
    String softApIp = "0.0.0.0";
    String localHostname = "piku.local";
    bool mdnsActive = false;
    bool staConfigured = false;
    String staSsid = "";
    String staStatus = "NOT_CONFIGURED";
    bool staConnected = false;
    String staIp = "0.0.0.0";
    int staRssi = 0;
    int staSignalQuality = 0;
  } _wifiTelemetry;

  void handleRoot();
  void handleCss();
  void handleJs();
  void handleData();
  void handlePing();
  void handleMode();
  void handleTurnMode();
  void handleSpeed();
  void handleCommand();
  void handleSensorHead();
  void handleAlarmEnable();
  void handleAlarmMute();
  void handleAlarmTest();
  void handleWifiScan();
  void handleWifiConnect();
  void handleWifiForget();
  void handleModeAuto();
  void handleModeManual();
  void handleControlForward();
  void handleControlBackward();
  void handleControlLeft();
  void handleControlRight();
  void handleControlStop();
  void applyMode(const String& mode);
  void applyTurnMode(const String& mode);
  void applyCommand(const String& command, unsigned long sequence = 0, bool sequenced = false);
  void configureSta(const String& ssid, const String& password, bool persist);
  void loadSavedStaCredentials(const char* fallbackSsid, const char* fallbackPassword);
  void clearStaCredentials();
  void updateWiFi();
  void refreshWiFiTelemetry(bool force = false);
  void applyCachedWiFiTelemetry();
  void beginStaAssociation();
  void updateMdns(bool staConnected);
  void setStaStateFromWiFiStatus();
  const char* staStateName(StaState state) const;
  void sendWifiScanJson();
  int calculateSignalQuality(int rssi) const;
  void addCoordinatorCors();
  bool controlOriginAllowed();
  void sendOk();
  void sendJson();
};

#endif
