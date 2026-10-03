#ifndef WEBDASHBOARD_H
#define WEBDASHBOARD_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include "PriorityWebServer.h"

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
  bool gasCritical = false;
  bool flameCritical = false;
  bool temperatureCritical = false;
  String flameState = "CLEAR";
  int flameDigitalLevel = HIGH;
  bool flameRawActive = false;
  bool flameSampleValid = false;
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
  bool manualFlameGasOverride = false;
  String manualMotionInhibitReason = "NONE";
  String autoNavigationState = "IDLE";
  String autoIrZoneState = "CLEAR";
  String autoAvoidanceSource = "NONE";
  String autoPreferredPath = "NONE";
  uint8_t autoObstacleRecoveryAttempts = 0;
  uint16_t autoBlockedRetryCount = 0;
  float autoTurnTargetDeg = 35.0f;
  float autoRelativeTurnDeg = 0.0f;
  String autoSelectedPath = "NONE";
  String autoTurnCompletionReason = "NONE";
  String autoScanLeftState = "NOT_SAMPLED";
  String autoScanFrontState = "NOT_SAMPLED";
  String autoScanRightState = "NOT_SAMPLED";
  float autoScanLeftDistanceCm = NAN;
  float autoScanFrontDistanceCm = NAN;
  float autoScanRightDistanceCm = NAN;
  bool autoScanLeftTrusted = false;
  bool autoScanFrontTrusted = false;
  bool autoScanRightTrusted = false;
  String autoHazardRecoveryState = "IDLE";
  unsigned long autoHazardClearElapsedMs = 0;
  unsigned long autoHazardClearRemainingMs = 0;
  String autoStuckState = "MONITORING";
  uint8_t autoStuckRecoveryAttempts = 0;
  String autoStuckLastReason = "NONE";
  String autoV2State = "OFF";
  uint8_t autoV2Attempts = 0;
  bool autoAssistanceRequired = false;
  String autoV2Reason = "NONE";
  bool autoRecoveryEpisodeActive = false;
  unsigned long autoRecoveryEpisodeStartedMs = 0;
  uint8_t autoRecoveryTurnsUsed = 0, autoRecoveryTurnsMax = 6, autoRecoveryIrPulses = 0;
  bool autoRecoveryIrActive = false, autoSonarHold = false;
  String autoRecoveryCommittedSide = "NONE", autoRecoveryTransition = "NONE";
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
  int softApChannel = 0;
  int softApClients = 0;
  uint32_t freeHeap = 0;
  uint32_t minimumFreeHeap = 0;
  uint32_t psramSize = 0;
  String resetReason;
  unsigned long maximumServiceLoopGapMs = 0;
  uint32_t wifiStaDisconnectCount = 0;
  uint32_t wifiApTransitionCount = 0;
  int wifiLastDisconnectReason = 0;
  uint32_t wifiLastEvent = 0;

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
  bool radioEnabled = true;
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
  using CommandAcceptedCallback = void (*)(const String&, unsigned long, unsigned long);
  using ModeAcceptedCallback = void (*)(const String&);
  using OverrideActionCallback = bool (*)(bool);

  WebDashboard();
  void begin(const char* ssid, const char* password, const char* staSsid = "", const char* staPassword = "");
  void loop();
  void updateBackground();
  void setControlCallbacks(CommandAcceptedCallback commandCallback, ModeAcceptedCallback modeCallback);
  void setOverrideCallback(OverrideActionCallback callback);
  void setManualOverrideState(bool enabled);
  bool sessionAlive() const;
  bool autoDashboardPollAlive() const;
  void invalidateControlSession();
  bool consumeControlActivity();
  void markCommandApplied(const String& command, unsigned long sequence, const String& stopReason);
  void markStopApplied(const String& stopReason);
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
  unsigned long pendingCommandSequence() const;
  IPAddress localIP() const;

private:
  PriorityWebServer _server;
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
  bool _staCredentialsPendingPersistence;
  unsigned long _lastStaAttemptMs;
  unsigned long _staRetryIntervalMs;
  bool _alarmEnabled;
  bool _alarmMuted;
  bool _alarmTestRequested;
  unsigned long _modeCommandCounter;
  unsigned long _lastCommandReceivedMs;
  unsigned long _lastCommandSequence;
  unsigned long _pendingCommandSequence;
  unsigned long _lastCommandAcceptedMs;
  unsigned long _lastCommandAppliedMs;
  String _lastAcceptedCommand;
  String _lastAppliedCommand;
  String _lastStopReason;
  unsigned long _httpErrorCount;
  unsigned long _wifiReconnectCount;
  bool _controlActivity;
  CommandAcceptedCallback _commandAcceptedCallback;
  ModeAcceptedCallback _modeAcceptedCallback;
  OverrideActionCallback _overrideActionCallback = nullptr;
  String _sessionToken;
  unsigned long _lastSessionHeartbeatMs = 0;
  unsigned long _lastAutoDashboardPollMs = 0;
  bool _requireFreshSessionAfterOverride = false;
  unsigned long _lastWiFiUpdateMs;
  unsigned long _lastWiFiTelemetryMs;
  unsigned long _lastDiagnosticsLogMs = 0;
  uint32_t _lastLoggedWifiEvent = 0;
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
    int softApChannel = 0;
    int softApClients = 0;
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
  void handleSession();
  void handleSessionHeartbeat();
  void handleOverride();
  bool validSessionToken();
  void handleSensorHead();
  void handleAlarmEnable();
  void handleAlarmMute();
  void handleAlarmTest();
  void handleWifiScan();
  void handleWifiConnect();
  void handleWifiForget();
  void handleWifiReconnect();
  void handleModeAuto();
  void handleModeManual();
  void handleControlForward();
  void handleControlBackward();
  void handleControlLeft();
  void handleControlRight();
  void handleControlStop();
  void applyMode(const String& mode);
  void applyTurnMode(const String& mode);
  bool applyCommand(const String& command, unsigned long sequence = 0, bool sequenced = false);
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
  bool wifiProvisioningAllowed();
  void prepareControlResponse();
  void sendError(int status, const char* contentType, const char* body);
  int calculateSignalQuality(int rssi) const;
  void addCoordinatorCors();
  bool controlOriginAllowed();
  void sendOk();
  void sendJson();
};

#endif
