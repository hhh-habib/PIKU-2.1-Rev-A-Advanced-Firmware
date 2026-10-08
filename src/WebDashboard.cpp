#include "WebDashboard.h"
#include "ControlOriginPolicy.h"
#include <ESPmDNS.h>
#include "RobotConfig.h"
#include "StaReconnectPolicy.h"
#include "WebDashboardAssets.h"
#include "WiFiConfig.h"

namespace {
  // Called on the Wi-Fi event task; never perform serial or HTTP work here.
  volatile uint32_t staDisconnectEvents = 0;
  volatile uint32_t apTransitionEvents = 0;
  volatile int lastStaDisconnectReason = 0;
  volatile uint32_t lastWifiEvent = 0;
  void recordWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    lastWifiEvent = static_cast<uint32_t>(event);
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      ++staDisconnectEvents;
      lastStaDisconnectReason = info.wifi_sta_disconnected.reason;
    } else if (event == ARDUINO_EVENT_WIFI_AP_START ||
               event == ARDUINO_EVENT_WIFI_AP_STOP ||
               event == ARDUINO_EVENT_WIFI_AP_STACONNECTED ||
               event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
      ++apTransitionEvents;
    }
  }
  String jsonFloat(float value, int decimals) {
    if (!isfinite(value)) {
      return "null";
    }
    return String(value, decimals);
  }

  const char* jsonBool(bool value) {
    return value ? "true" : "false";
  }

  String jsonEscape(const String& value) {
    String escaped;
    escaped.reserve(value.length() + 4);
    for (unsigned int i = 0; i < value.length(); i++) {
      char c = value.charAt(i);
      if (c == '"' || c == '\\') {
        escaped += '\\';
      }
      escaped += c;
    }
    return escaped;
  }

  bool hasText(const char* value) {
    return value != nullptr && value[0] != '\0';
  }
}

WebDashboard::WebDashboard()
  : _server(80),
    _initialized(false),
    _preferences(),
    _staState(StaState::NotConfigured),
    _staConfigured(false),
    _staConnectStarted(false),
    _scanStarted(false),
    _lastStaConnected(false),
    _mdnsStarted(false),
    _staSsid(""),
    _staPassword(""),
    _staCredentialsPendingPersistence(false),
    _lastStaAttemptMs(0),
    _staRetryIntervalMs(RobotConfig::WIFI_STA_INITIAL_RETRY_MS),
    _alarmEnabled(true),
    _alarmMuted(false),
    _alarmTestRequested(false),
    _modeCommandCounter(0),
    _lastCommandReceivedMs(0),
    _lastCommandSequence(0),
    _pendingCommandSequence(0),
    _lastCommandAcceptedMs(0),
    _lastCommandAppliedMs(0),
    _httpErrorCount(0),
    _wifiReconnectCount(0),
    _controlActivity(false),
    _commandAcceptedCallback(nullptr),
    _modeAcceptedCallback(nullptr),
    _lastWiFiUpdateMs(0),
    _lastWiFiTelemetryMs(0),
    _pendingSpeedPercent(-1),
    _pendingManualSensorHeadCommand(ManualSensorHeadCommand::None) {}

void WebDashboard::begin(const char* ssid, const char* password, const char* staSsid, const char* staPassword) {
  loadSavedStaCredentials(staSsid, staPassword);
  WiFi.onEvent(recordWifiEvent);

  WiFi.mode(_staConfigured ? WIFI_AP_STA : WIFI_AP);
  // Motors dominate the robot power budget; disabling modem sleep favors
  // predictable dashboard command latency over small ESP32 energy savings.
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.softAP(ssid, password);
  delay(100);
  refreshWiFiTelemetry(true);

  if (_staConfigured) {
    _staState = StaState::Connecting;
    beginStaAssociation();
    _staConnectStarted = true;
    _lastStaAttemptMs = millis();
    _staRetryIntervalMs = RobotConfig::WIFI_STA_INITIAL_RETRY_MS;
  }

  _server.on("/", [this]() { handleRoot(); });
  _server.on("/dashboard.css", [this]() { handleCss(); });
  _server.on("/dashboard.js", [this]() { handleJs(); });
  _server.on("/data", [this]() { handleData(); });
  _server.on("/ping", [this]() { handlePing(); });
  _server.on("/mode", [this]() { handleMode(); });
  _server.on("/turn-mode", [this]() { handleTurnMode(); });
  _server.on("/speed", [this]() { handleSpeed(); });
  _server.on("/cmd", [this]() { handleCommand(); });
  _server.on("/control-session", HTTP_POST, [this]() { handleSession(); });
  _server.on("/control-heartbeat", HTTP_POST, [this]() { handleSessionHeartbeat(); });
  _server.on("/manual-override", HTTP_POST, [this]() { handleOverride(); });
  _server.on("/sensor-head", [this]() { handleSensorHead(); });
  _server.on("/alarm/enable", [this]() { handleAlarmEnable(); });
  _server.on("/alarm/mute", [this]() { handleAlarmMute(); });
  _server.on("/alarm/test", [this]() { handleAlarmTest(); });
  _server.on("/wifi/scan", [this]() { handleWifiScan(); });
  _server.on("/wifi/connect", HTTP_POST, [this]() { handleWifiConnect(); });
  _server.on("/wifi/forget", HTTP_POST, [this]() { handleWifiForget(); });
  _server.on("/wifi/reconnect", HTTP_POST, [this]() { handleWifiReconnect(); });
  _server.on("/mode/auto", [this]() { handleModeAuto(); });
  _server.on("/mode/manual", [this]() { handleModeManual(); });
  _server.on("/control/forward", [this]() { handleControlForward(); });
  _server.on("/control/backward", [this]() { handleControlBackward(); });
  _server.on("/control/left", [this]() { handleControlLeft(); });
  _server.on("/control/right", [this]() { handleControlRight(); });
  _server.on("/control/stop", [this]() { handleControlStop(); });
  _server.onNotFound([this]() {
    sendError(404, "application/json", R"json({"ok":false,"error":"not_found"})json");
  });
  const char* collectedHeaders[] = {"Origin"};
  _server.collectHeaders(collectedHeaders, 1);
  _server.begin();
  _initialized = true;

  Serial.println("[WebDashboard] Access Point started");
  Serial.print("[WebDashboard] SSID: ");
  Serial.println(ssid);
  Serial.print("[WebDashboard] AP IP: ");
  Serial.println(WiFi.softAPIP());
  if (_staConfigured) {
    Serial.println("[WebDashboard] STA connection configured.");
  }
}

void WebDashboard::loop() {
  if (_initialized) {
    _server.handleClient();
  }
}

void WebDashboard::updateBackground() {
  updateWiFi();
  const uint32_t event = lastWifiEvent;
  _data.wifiStaDisconnectCount = staDisconnectEvents;
  _data.wifiApTransitionCount = apTransitionEvents;
  _data.wifiLastDisconnectReason = lastStaDisconnectReason;
  _data.wifiLastEvent = event;
  if (event != _lastLoggedWifiEvent) {
    _lastLoggedWifiEvent = event;
    Serial.printf("[Diag] wifi_event=%lu sta_disconnects=%lu reason=%d ap_transitions=%lu\n",
                  static_cast<unsigned long>(event),
                  static_cast<unsigned long>(_data.wifiStaDisconnectCount),
                  _data.wifiLastDisconnectReason,
                  static_cast<unsigned long>(_data.wifiApTransitionCount));
  }
  const unsigned long now = millis();
  if (now - _lastDiagnosticsLogMs >= 30000) {
    _lastDiagnosticsLogMs = now;
    Serial.printf("[Diag] fw=%s uptime_ms=%lu sta=%s rssi=%d reconnects=%lu heap=%lu min_heap=%lu loop_gap_max_ms=%lu http_errors=%lu http_timeouts=%lu\n",
                  RobotConfig::FIRMWARE_VERSION, now, _data.staConnected ? "UP" : "DOWN",
                  _data.staRssi, static_cast<unsigned long>(_wifiReconnectCount),
                  static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getMinFreeHeap()),
                  _data.maximumServiceLoopGapMs, _httpErrorCount,
                  static_cast<unsigned long>(_server.timeoutCount()));
  }
}

void WebDashboard::setControlCallbacks(CommandAcceptedCallback commandCallback,
                                       ModeAcceptedCallback modeCallback) {
  _commandAcceptedCallback = commandCallback;
  _modeAcceptedCallback = modeCallback;
}

void WebDashboard::setOverrideCallback(OverrideActionCallback callback) {
  _overrideActionCallback = callback;
}

void WebDashboard::setManualOverrideState(bool enabled) {
  _data.manualFlameGasOverride = enabled;
}

void WebDashboard::invalidateControlSession() {
  _sessionToken = "";
  _lastSessionHeartbeatMs = 0;
  _requireFreshSessionAfterOverride = true;
}

bool WebDashboard::autoDashboardPollAlive() const {
  return _lastAutoDashboardPollMs != 0 &&
      millis() - _lastAutoDashboardPollMs <
      RobotConfig::AUTO_V2_DASHBOARD_LINK_TIMEOUT_MS;
}

bool WebDashboard::sessionAlive() const {
  return _sessionToken.length() > 0 && _lastSessionHeartbeatMs != 0 &&
         millis() - _lastSessionHeartbeatMs < RobotConfig::MANUAL_SESSION_TIMEOUT_MS;
}

bool WebDashboard::validSessionToken() {
  return sessionAlive() && _server.hasArg("token") &&
         _server.arg("token") == _sessionToken;
}

bool WebDashboard::consumeControlActivity() {
  const bool active = _controlActivity;
  _controlActivity = false;
  return active;
}

void WebDashboard::markCommandApplied(const String& command,
                                      unsigned long sequence,
                                      const String& stopReason) {
  _lastAppliedCommand = command;
  _lastCommandAppliedMs = millis();
  if (sequence > _lastCommandSequence) {
    _lastCommandSequence = sequence;
  }
  if (command == "STOP" && stopReason.length() > 0) {
    _lastStopReason = stopReason;
  }
}

void WebDashboard::markStopApplied(const String& stopReason) {
  markCommandApplied("STOP", _lastCommandSequence, stopReason);
}

void WebDashboard::setData(const DashboardData& data) {
  _data = data;
  _data.wifiStaDisconnectCount = staDisconnectEvents;
  _data.wifiApTransitionCount = apTransitionEvents;
  _data.wifiLastDisconnectReason = lastStaDisconnectReason;
  _data.wifiLastEvent = lastWifiEvent;
  refreshWiFiTelemetry();
}

void WebDashboard::setNavigationDecision(const String& decision) {
  _data.navigationDecision = decision;
}

void WebDashboard::setControlMode(const String& mode) {
  _data.controlMode = mode;
}

void WebDashboard::setTurnMode(const String& mode) {
  _data.turnMode = mode;
}

void WebDashboard::setPendingCommand(const String& command) {
  _pendingCommand = command;
}

const String& WebDashboard::getControlMode() const {
  return _data.controlMode;
}

const String& WebDashboard::getTurnMode() const {
  return _data.turnMode;
}

const String& WebDashboard::getPendingCommand() const {
  return _pendingCommand;
}

ManualSensorHeadCommand WebDashboard::consumeManualSensorHeadCommand() {
  ManualSensorHeadCommand command = _pendingManualSensorHeadCommand;
  _pendingManualSensorHeadCommand = ManualSensorHeadCommand::None;
  return command;
}

bool WebDashboard::consumeSpeedPercent(int& speedPercent) {
  if (_pendingSpeedPercent < 0) {
    return false;
  }
  speedPercent = _pendingSpeedPercent;
  _pendingSpeedPercent = -1;
  return true;
}
bool WebDashboard::isAlarmEnabled() const {
  return _alarmEnabled;
}

bool WebDashboard::isAlarmMuted() const {
  return _alarmMuted || !_alarmEnabled;
}

bool WebDashboard::consumeAlarmTestRequest() {
  bool requested = _alarmTestRequested;
  _alarmTestRequested = false;
  return requested;
}

unsigned long WebDashboard::modeCommandCounter() const {
  return _modeCommandCounter;
}

unsigned long WebDashboard::lastCommandReceivedMs() const {
  return _lastCommandReceivedMs;
}

unsigned long WebDashboard::pendingCommandSequence() const {
  return _pendingCommandSequence;
}

IPAddress WebDashboard::localIP() const {
  return WiFi.softAPIP();
}

void WebDashboard::handleRoot() {
  _server.send_P(200, "text/html", DASHBOARD_HTML);
}

void WebDashboard::handleCss() {
  _server.send_P(200, "text/css", DASHBOARD_CSS);
}

void WebDashboard::handleJs() {
  _server.send_P(200, "application/javascript", DASHBOARD_JS);
}

void WebDashboard::handleData() {
  if (_data.controlMode == "AUTO") _lastAutoDashboardPollMs = millis();
  // Full telemetry and lightweight ping both prove browser session liveness
  // independently of held movement refreshes.
  if (_server.hasArg("token") && validSessionToken())
    _lastSessionHeartbeatMs = millis();
  addCoordinatorCors();
  _server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  _server.sendHeader("Pragma", "no-cache");
  _server.sendHeader("Expires", "0");
  refreshWiFiTelemetry();
  _server.beginTelemetryResponse();
  sendJson();
  _server.endTelemetryResponse();
}

void WebDashboard::handlePing() {
  if (_data.controlMode == "AUTO") _lastAutoDashboardPollMs = millis();
  if (_server.hasArg("token") && validSessionToken())
    _lastSessionHeartbeatMs = millis();
  addCoordinatorCors();
  _server.sendHeader("Cache-Control", "no-store");
  const String telemetryAge = _data.uptimeMs == 0
      ? "null"
      : String(millis() - _data.uptimeMs);
  const String body = String("{\"ok\":true,\"node_id\":\"") + RobotConfig::NODE_ID +
                      "\",\"node_ms\":" + millis() +
                      ",\"telemetry_age_ms\":" + telemetryAge + "}";
  _server.send(200, "application/json", body);
}

void WebDashboard::handleMode() {
  if (!controlOriginAllowed()) return;
  prepareControlResponse();
  if (!_server.hasArg("set")) {
    sendError(400, "text/plain", "ERR");
    return;
  }

  String mode = _server.arg("set");
  mode.toUpperCase();
  if (mode != "AUTO" && mode != "MANUAL") {
    sendError(400, "text/plain", "ERR");
    return;
  }

  applyMode(mode);
  sendOk();
}

void WebDashboard::handleTurnMode() {
  if (!controlOriginAllowed()) return;
  prepareControlResponse();
  if (!_server.hasArg("set")) {
    sendError(400, "text/plain", "ERR");
    return;
  }

  String mode = _server.arg("set");
  mode.toUpperCase();
  if (mode != "PIVOT" && mode != "SPIN") {
    sendError(400, "text/plain", "ERR");
    return;
  }

  applyTurnMode(mode);
  sendOk();
}

void WebDashboard::handleSpeed() {
  if (!controlOriginAllowed()) return;
  prepareControlResponse();
  if (!_server.hasArg("set")) {
    sendError(400, "text/plain", "ERR");
    return;
  }

  const String value = _server.arg("set");
  if (value.length() == 0 || value.length() > 3) {
    sendError(400, "text/plain", "ERR");
    return;
  }
  for (unsigned int i = 0; i < value.length(); i++) {
    if (!isDigit(value.charAt(i))) {
      sendError(400, "text/plain", "ERR");
      return;
    }
  }

  const int speedPercent = value.toInt();
  if (speedPercent < 0 || speedPercent > 100) {
    sendError(400, "text/plain", "ERR");
    return;
  }
  _pendingSpeedPercent = speedPercent;
  _controlActivity = true;
  sendOk();
}
void WebDashboard::handleSession() {
  if (!controlOriginAllowed()) return;
  if (_data.controlMode != "MANUAL") {
    sendError(409, "application/json", "{\"ok\":false,\"error\":\"manual_required\"}");
    return;
  }
  applyCommand("RELEASE"); // replace session only after stopping old movement; preserve permission
  _sessionToken = String(esp_random(), HEX) + String(esp_random(), HEX);
  _lastSessionHeartbeatMs = millis();
  _requireFreshSessionAfterOverride = false;
  _server.send(200, "application/json",
               String("{\"ok\":true,\"token\":\"") + _sessionToken +
               "\",\"timeout_ms\":" + RobotConfig::MANUAL_SESSION_TIMEOUT_MS + "}");
}

void WebDashboard::handleSessionHeartbeat() {
  if (!controlOriginAllowed()) return;
  if (!validSessionToken() || _data.controlMode != "MANUAL") {
    sendError(409, "application/json", "{\"ok\":false,\"error\":\"session_expired\"}");
    return;
  }
  _lastSessionHeartbeatMs = millis();
  sendOk();
}

void WebDashboard::handleOverride() {
  if (!controlOriginAllowed()) return;
  if (!validSessionToken() || _data.controlMode != "MANUAL" ||
      _overrideActionCallback == nullptr) {
    sendError(409, "application/json", "{\"ok\":false,\"error\":\"session_or_mode_rejected\"}");
    return;
  }
  const String action = _server.arg("set");
  if (action != "on" && action != "off") {
    sendError(400, "application/json", "{\"ok\":false,\"error\":\"invalid_action\"}");
    return;
  }
  if (action == "on" && _server.arg("confirm") != "1") {
    sendError(409, "application/json", "{\"ok\":false,\"error\":\"confirmation_required\"}");
    return;
  }
  if (!_overrideActionCallback(action == "on")) {
    sendError(409, "application/json", "{\"ok\":false,\"error\":\"safety_rejected\"}");
    return;
  }
  _server.send(200, "application/json",
               action == "on" ? "{\"ok\":true,\"enabled\":true}" :
                                "{\"ok\":true,\"enabled\":false}");
}

void WebDashboard::handleCommand() {
  if (!controlOriginAllowed()) return;
  prepareControlResponse();
  if (!_server.hasArg("move")) {
    sendError(400, "text/plain", "ERR");
    return;
  }

  String command = _server.arg("move");
  command.toUpperCase();
  if (command != "FORWARD" && command != "BACKWARD" && command != "LEFT" &&
      command != "RIGHT" && command != "STOP" && command != "RELEASE") {
    sendError(400, "text/plain", "ERR");
    return;
  }

  bool sequenced = _server.hasArg("seq");
  unsigned long sequence = 0;
  if (sequenced) {
    String sequenceText = _server.arg("seq");
    if (sequenceText.length() == 0) {
      sendError(400, "text/plain", "ERR");
      return;
    }
    for (unsigned int i = 0; i < sequenceText.length(); i++) {
      if (!isDigit(sequenceText.charAt(i))) {
        sendError(400, "text/plain", "ERR");
        return;
      }
    }
    sequence = strtoul(sequenceText.c_str(), nullptr, 10);
    if (sequence == 0) {
      sendError(400, "text/plain", "ERR");
      return;
    }
  }

  if ((_data.manualFlameGasOverride || _requireFreshSessionAfterOverride) &&
      command != "STOP" &&
      command != "RELEASE" && !validSessionToken()) {
    sendError(409, "text/plain", "SESSION_REJECTED");
    return;
  }
  // This is MANUAL retirement, not a new explicit AUTO operator STOP.
  // Untagged STOP retains its all-mode, fail-safe sequence semantics.
  if (command == "STOP" && _server.arg("retire") == "manual" &&
      _data.controlMode != "MANUAL") { sendOk(); return; }
  if (applyCommand(command, sequence, sequenced)) {
    sendOk();
  } else {
    sendError(409, "text/plain", "STALE_OR_MODE_REJECTED");
  }
}

void WebDashboard::handleSensorHead() {
  if (!controlOriginAllowed()) return;
  if (!_server.hasArg("set")) {
    sendError(400, "text/plain", "Missing set value. Use LEFT, FRONT, or RIGHT.");
    return;
  }

  String orientation = _server.arg("set");
  ManualSensorHeadCommand command = ManualSensorHeadCommand::None;
  if (orientation == "LEFT") {
    command = ManualSensorHeadCommand::Left;
  } else if (orientation == "FRONT") {
    command = ManualSensorHeadCommand::Front;
  } else if (orientation == "RIGHT") {
    command = ManualSensorHeadCommand::Right;
  } else {
    sendError(400, "text/plain", "Invalid set value. Use LEFT, FRONT, or RIGHT.");
    return;
  }

  if (_data.controlMode != "MANUAL") {
    sendError(409, "text/plain", "Manual mode required.");
    return;
  }

  // One pending slot prevents overlapping operations; the latest valid request wins.
  _pendingManualSensorHeadCommand = command;
  _server.send(202, "text/plain", "ACCEPTED");
}

void WebDashboard::handleAlarmEnable() {
  if (!controlOriginAllowed()) return;
  _alarmEnabled = true;
  _alarmMuted = false;
  sendOk();
}

void WebDashboard::handleAlarmMute() {
  if (!controlOriginAllowed()) return;
  _alarmEnabled = false;
  _alarmMuted = true;
  sendOk();
}

void WebDashboard::handleAlarmTest() {
  if (!controlOriginAllowed()) return;
  _alarmTestRequested = true;
  sendOk();
}

void WebDashboard::handleWifiScan() {
  if (!controlOriginAllowed()) return;
  if (!wifiProvisioningAllowed()) {
    sendError(409, "application/json", R"json({"ok":false,"error":"robot_must_be_stopped_in_manual"})json");
    return;
  }
  sendWifiScanJson();
}

void WebDashboard::handleWifiConnect() {
  if (!controlOriginAllowed()) return;
  if (!wifiProvisioningAllowed()) {
    sendError(409, "application/json", R"json({"ok":false,"error":"robot_must_be_stopped_in_manual"})json");
    return;
  }
  if (!_server.hasArg("ssid")) {
    ++_httpErrorCount;
    _server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_ssid\"}");
    return;
  }

  String ssid = _server.arg("ssid");
  String password = _server.hasArg("password") ? _server.arg("password") : "";
  ssid.trim();
  if (ssid.length() == 0) {
    ++_httpErrorCount;
    _server.send(400, "application/json", "{\"ok\":false,\"error\":\"empty_ssid\"}");
    return;
  }

  if (ssid.length() > 32 || password.length() > 64) {
    sendError(400, "application/json", R"json({"ok":false,"error":"invalid_credential_length"})json");
    return;
  }

  configureSta(ssid, password, true);
  _server.send(200, "application/json", "{\"ok\":true}");
}

void WebDashboard::handleWifiForget() {
  if (!controlOriginAllowed()) return;
  if (!wifiProvisioningAllowed()) {
    sendError(409, "application/json", R"json({"ok":false,"error":"robot_must_be_stopped_in_manual"})json");
    return;
  }
  clearStaCredentials();
  _server.send(200, "application/json", "{\"ok\":true}");
}

void WebDashboard::handleWifiReconnect() {
  if (!controlOriginAllowed()) return;
  if (!wifiProvisioningAllowed()) {
    sendError(409, "application/json", R"json({"ok":false,"error":"robot_must_be_stopped_in_manual"})json");
    return;
  }
  if (!_staConfigured) {
    sendError(409, "application/json", R"json({"ok":false,"error":"wifi_not_configured"})json");
    return;
  }
  WiFi.disconnect(false, false);
  _staState = StaState::Connecting;
  beginStaAssociation();
  _staConnectStarted = true;
  _lastStaAttemptMs = millis();
  _staRetryIntervalMs = RobotConfig::WIFI_STA_INITIAL_RETRY_MS;
  ++_wifiReconnectCount;
  _server.send(202, "application/json", R"json({"ok":true,"state":"CONNECTING"})json");
}

void WebDashboard::handleModeAuto() {
  if (!controlOriginAllowed()) return;
  applyMode("AUTO");
  sendOk();
}

void WebDashboard::handleModeManual() {
  if (!controlOriginAllowed()) return;
  applyMode("MANUAL");
  sendOk();
}

void WebDashboard::handleControlForward() {
  if (!controlOriginAllowed()) return;
  prepareControlResponse();
  if (applyCommand("FORWARD")) sendOk(); else sendError(409, "text/plain", "MODE_REJECTED");
}

void WebDashboard::handleControlBackward() {
  if (!controlOriginAllowed()) return;
  prepareControlResponse();
  if (applyCommand("BACKWARD")) sendOk(); else sendError(409, "text/plain", "MODE_REJECTED");
}

void WebDashboard::handleControlLeft() {
  if (!controlOriginAllowed()) return;
  prepareControlResponse();
  if (applyCommand("LEFT")) sendOk(); else sendError(409, "text/plain", "MODE_REJECTED");
}

void WebDashboard::handleControlRight() {
  if (!controlOriginAllowed()) return;
  prepareControlResponse();
  if (applyCommand("RIGHT")) sendOk(); else sendError(409, "text/plain", "MODE_REJECTED");
}

void WebDashboard::handleControlStop() {
  if (!controlOriginAllowed()) return;
  prepareControlResponse();
  applyCommand("STOP");
  sendOk();
}

void WebDashboard::applyMode(const String& mode) {
  _data.controlMode = mode;
  _pendingCommand = "";
  _pendingManualSensorHeadCommand = ManualSensorHeadCommand::None;
  _modeCommandCounter++;
  _controlActivity = true;
  if (_modeAcceptedCallback != nullptr) {
    _modeAcceptedCallback(mode);
  }
}

void WebDashboard::applyTurnMode(const String& mode) {
  _data.turnMode = mode;
  _controlActivity = true;
}

bool WebDashboard::applyCommand(const String& command, unsigned long sequence, bool sequenced) {
  // STOP is an all-mode operator intervention. Directional commands remain
  // Manual-only and continue through the existing command handler.
  if (command == "STOP" || _data.controlMode == "MANUAL") {
    if ((_data.manualFlameGasOverride || _requireFreshSessionAfterOverride) &&
        command != "STOP" &&
        command != "RELEASE" && !validSessionToken()) return false;
    if (sequenced) {
      // The browser assigns increasing sequence numbers. A STOP that reaches
      // the ESP32 first therefore makes every older in-flight direction stale.
      if (sequence <= _lastCommandSequence) {
        // STOP is fail-safe and remains accepted even from a newly opened
        // dashboard that has not synchronized the current sequence yet.
        if (command != "STOP") {
          return false;
        }
      } else {
        _lastCommandSequence = sequence;
      }
    } else {
      _lastCommandSequence++;
      if (_lastCommandSequence == 0) {
        _lastCommandSequence = 1;
      }
    }
    _pendingCommand = command;
    _pendingCommandSequence = _lastCommandSequence;
    _lastCommandReceivedMs = millis();
    _lastCommandAcceptedMs = _lastCommandReceivedMs;
    _lastAcceptedCommand = command;
    _controlActivity = true;
    if (_commandAcceptedCallback != nullptr) {
      _commandAcceptedCallback(command, _pendingCommandSequence, _lastCommandAcceptedMs);
    }
    return true;
  }
  return false;
}

void WebDashboard::configureSta(const String& ssid, const String& password, bool persist) {
  bool sameSsid = _staConfigured && ssid == _staSsid;
  bool passwordProvided = password.length() > 0;
  String nextPassword = (sameSsid && !passwordProvided) ? _staPassword : password;
  bool associationActive = _staState == StaState::Connecting || WiFi.status() == WL_CONNECTED;

  if (sameSsid && !passwordProvided && associationActive) {
    refreshWiFiTelemetry(true);
    return;
  }

  bool credentialsChanged = !sameSsid || nextPassword != _staPassword;
  _staSsid = ssid;
  _staPassword = nextPassword;
  _staConfigured = _staSsid.length() > 0;

  // Commit a provisioning candidate only after association succeeds. Failed
  // credentials cannot replace the last known-good NVS network.
  _staCredentialsPendingPersistence = persist;

  if (WiFi.getMode() != WIFI_AP_STA) {
    WiFi.mode(WIFI_AP_STA);
  }
  if (_staConfigured) {
    if (credentialsChanged || WiFi.status() == WL_CONNECTED) {
      WiFi.disconnect(false, false);
    }
    _staState = StaState::Connecting;
    beginStaAssociation();
    _staConnectStarted = true;
    _lastStaAttemptMs = millis();
    _staRetryIntervalMs = RobotConfig::WIFI_STA_INITIAL_RETRY_MS;
  } else {
    _staState = StaState::NotConfigured;
    _staConnectStarted = false;
    _lastStaAttemptMs = 0;
  }
  refreshWiFiTelemetry(true);
}

void WebDashboard::loadSavedStaCredentials(const char* fallbackSsid, const char* fallbackPassword) {
  _preferences.begin("piku-wifi", true);
  String savedSsid = _preferences.getString("ssid", "");
  String savedPassword = _preferences.getString("password", "");
  _preferences.end();

  _staCredentialsPendingPersistence = false;

  if (savedSsid.length() > 0) {
    _staSsid = savedSsid;
    _staPassword = savedPassword;
  } else if (hasText(fallbackSsid)) {
    _staSsid = fallbackSsid;
    _staPassword = hasText(fallbackPassword) ? fallbackPassword : "";
  } else {
    _staSsid = "";
    _staPassword = "";
  }
  _staConfigured = _staSsid.length() > 0;
  _data.staConfigured = _staConfigured;
  _data.staSsid = _staConfigured ? _staSsid : "";
}

void WebDashboard::clearStaCredentials() {
  _preferences.begin("piku-wifi", false);
  _preferences.remove("ssid");
  _preferences.remove("password");
  _preferences.end();

  _staSsid = "";
  _staPassword = "";
  _staCredentialsPendingPersistence = false;
  _staConfigured = false;
  _staConnectStarted = false;
  _staState = StaState::NotConfigured;
  _lastStaAttemptMs = 0;
  _staRetryIntervalMs = RobotConfig::WIFI_STA_INITIAL_RETRY_MS;
  updateMdns(false);
  WiFi.disconnect(false, true);
  WiFi.mode(WIFI_AP);
  refreshWiFiTelemetry(true);
}

void WebDashboard::updateWiFi() {
  unsigned long now = millis();
  if (_lastWiFiUpdateMs != 0 &&
      now - _lastWiFiUpdateMs < RobotConfig::WIFI_STATUS_INTERVAL_MS) {
    return;
  }
  _lastWiFiUpdateMs = now;

  if (!_staConfigured) {
    _staState = StaState::NotConfigured;
    refreshWiFiTelemetry(true);
    return;
  }

  setStaStateFromWiFiStatus();
  if (WiFi.status() == WL_CONNECTED) {
    _staConnectStarted = false;
    _staRetryIntervalMs = RobotConfig::WIFI_STA_INITIAL_RETRY_MS;
    if (_staCredentialsPendingPersistence) {
      _preferences.begin("piku-wifi", false);
      _preferences.putString("ssid", _staSsid);
      _preferences.putString("password", _staPassword);
      _preferences.end();
      _staCredentialsPendingPersistence = false;
    }
    updateMdns(true);
    refreshWiFiTelemetry(true);
    return;
  }

  updateMdns(false);

  if (staRetryDue(now, _lastStaAttemptMs, _staRetryIntervalMs)) {
    if (WiFi.getMode() != WIFI_AP_STA) WiFi.mode(WIFI_AP_STA);
    _staState = StaState::Connecting;
    if (!WiFi.reconnect()) beginStaAssociation();
    _staConnectStarted = true;
    _lastStaAttemptMs = now;
    _staRetryIntervalMs = staNextRetryInterval(
        _staRetryIntervalMs, RobotConfig::WIFI_STA_MAX_RETRY_MS);
    ++_wifiReconnectCount;
  }
  refreshWiFiTelemetry(true);
}

void WebDashboard::refreshWiFiTelemetry(bool force) {
  unsigned long now = millis();
  if (!force && _lastWiFiTelemetryMs != 0 &&
      now - _lastWiFiTelemetryMs < RobotConfig::WIFI_STATUS_INTERVAL_MS) {
    applyCachedWiFiTelemetry();
    return;
  }

  _lastWiFiTelemetryMs = now;
  _wifiTelemetry.softApIp = WiFi.softAPIP().toString();
  _wifiTelemetry.softApChannel = WiFi.channel();
  _wifiTelemetry.softApClients = WiFi.softAPgetStationNum();
  _wifiTelemetry.localHostname = "piku.local";
  _wifiTelemetry.mdnsActive = _mdnsStarted && WiFi.status() == WL_CONNECTED;
  _wifiTelemetry.staConfigured = _staConfigured;
  _wifiTelemetry.staSsid = _staConfigured ? _staSsid : "";
  setStaStateFromWiFiStatus();
  _wifiTelemetry.staStatus = staStateName(_staState);
  _wifiTelemetry.staConnected = _staConfigured && WiFi.status() == WL_CONNECTED;
  if (_wifiTelemetry.staConnected) {
    _wifiTelemetry.staIp = WiFi.localIP().toString();
    _wifiTelemetry.staRssi = WiFi.RSSI();
    _wifiTelemetry.staSignalQuality = calculateSignalQuality(_wifiTelemetry.staRssi);
  } else {
    _wifiTelemetry.staIp = "0.0.0.0";
    _wifiTelemetry.staRssi = 0;
    _wifiTelemetry.staSignalQuality = 0;
  }
  applyCachedWiFiTelemetry();
}

void WebDashboard::applyCachedWiFiTelemetry() {
  _data.softApIp = _wifiTelemetry.softApIp;
  _data.localHostname = _wifiTelemetry.localHostname;
  _data.mdnsActive = _wifiTelemetry.mdnsActive;
  _data.staConfigured = _wifiTelemetry.staConfigured;
  _data.staSsid = _wifiTelemetry.staSsid;
  _data.staStatus = _wifiTelemetry.staStatus;
  _data.staConnected = _wifiTelemetry.staConnected;
  _data.staIp = _wifiTelemetry.staIp;
  _data.staRssi = _wifiTelemetry.staRssi;
  _data.staSignalQuality = _wifiTelemetry.staSignalQuality;
  _data.softApChannel = _wifiTelemetry.softApChannel;
  _data.softApClients = _wifiTelemetry.softApClients;
}

void WebDashboard::beginStaAssociation() {
  if (_staSsid == WiFiConfig::COORDINATOR_SSID) {
    const IPAddress localIp(WiFiConfig::COORDINATOR_STA_IP[0],
                            WiFiConfig::COORDINATOR_STA_IP[1],
                            WiFiConfig::COORDINATOR_STA_IP[2],
                            WiFiConfig::COORDINATOR_STA_IP[3]);
    const IPAddress gateway(WiFiConfig::COORDINATOR_GATEWAY[0],
                            WiFiConfig::COORDINATOR_GATEWAY[1],
                            WiFiConfig::COORDINATOR_GATEWAY[2],
                            WiFiConfig::COORDINATOR_GATEWAY[3]);
    const IPAddress subnet(WiFiConfig::COORDINATOR_SUBNET[0],
                           WiFiConfig::COORDINATOR_SUBNET[1],
                           WiFiConfig::COORDINATOR_SUBNET[2],
                           WiFiConfig::COORDINATOR_SUBNET[3]);
    if (!WiFi.config(localIp, gateway, subnet, gateway)) {
      Serial.println("[WebDashboard] Coordinator static STA configuration failed.");
    }
  } else {
    WiFi.config(IPAddress(), IPAddress(), IPAddress());
  }

  if (_staPassword.length() == 0) {
    WiFi.begin(_staSsid.c_str());
  } else {
    WiFi.begin(_staSsid.c_str(), _staPassword.c_str());
  }
}

void WebDashboard::updateMdns(bool staConnected) {
  if (!staConnected) {
    if (_mdnsStarted || _lastStaConnected) {
      MDNS.end();
    }
    _mdnsStarted = false;
    _lastStaConnected = false;
    return;
  }

  if (_mdnsStarted && _lastStaConnected) {
    return;
  }

  if (_mdnsStarted) {
    MDNS.end();
    _mdnsStarted = false;
  }

  if (MDNS.begin("piku")) {
    MDNS.addService("http", "tcp", 80);
    _mdnsStarted = true;
    Serial.println("[WebDashboard] mDNS started: http://piku.local");
  } else {
    _mdnsStarted = false;
    Serial.println("[WebDashboard] mDNS start failed.");
  }
  _lastStaConnected = true;
}

void WebDashboard::setStaStateFromWiFiStatus() {
  if (!_staConfigured) {
    _staState = StaState::NotConfigured;
    return;
  }
  if (_scanStarted && WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
    _staState = StaState::Scanning;
    return;
  }

  wl_status_t status = WiFi.status();
  switch (status) {
    case WL_CONNECTED:
      _staState = StaState::Connected;
      return;
    case WL_CONNECT_FAILED:
      _staState = StaState::AuthFailed;
      _staConnectStarted = false;
      return;
    case WL_NO_SSID_AVAIL:
      _staState = StaState::NoAp;
      _staConnectStarted = false;
      return;
    case WL_IDLE_STATUS:
      _staState = _staConnectStarted ? StaState::Connecting : StaState::Disconnected;
      return;
    case WL_DISCONNECTED:
    default:
      _staState = _staConnectStarted ? StaState::Connecting : StaState::Disconnected;
      return;
  }
}

const char* WebDashboard::staStateName(StaState state) const {
  switch (state) {
    case StaState::NotConfigured: return "NOT_CONFIGURED";
    case StaState::Connecting: return "CONNECTING";
    case StaState::Connected: return "CONNECTED";
    case StaState::AuthFailed: return "AUTH_FAILED";
    case StaState::NoAp: return "NO_AP";
    case StaState::Disconnected: return "DISCONNECTED";
    case StaState::Scanning: return "SCANNING";
  }
  return "DISCONNECTED";
}

void WebDashboard::sendWifiScanJson() {
  if (WiFi.status() == WL_CONNECTED) {
    if (_scanStarted) {
      WiFi.scanDelete();
      _scanStarted = false;
    }
    refreshWiFiTelemetry(true);
    _server.send(200, "application/json",
                 "{\"running\":false,\"blocked\":true,\"reason\":\"STA_CONNECTED\",\"networks\":[]}");
    return;
  }

  int scanStatus = WiFi.scanComplete();
  if (scanStatus == WIFI_SCAN_FAILED) {
    _staState = StaState::Scanning;
    WiFi.scanNetworks(true, true);
    _scanStarted = true;
    _server.send(200, "application/json", "{\"running\":true,\"networks\":[]}");
    return;
  }
  if (scanStatus == WIFI_SCAN_RUNNING) {
    _server.send(200, "application/json", "{\"running\":true,\"networks\":[]}");
    return;
  }

  if (!_scanStarted && scanStatus < 0) {
    _staState = StaState::Scanning;
    WiFi.scanNetworks(true, true);
    _scanStarted = true;
    _server.send(200, "application/json", "{\"running\":true,\"networks\":[]}");
    return;
  }

  String json;
  json.reserve(512);
  json = "{\"running\":false,\"networks\":[";
  int count = scanStatus > 0 ? scanStatus : 0;
  for (int i = 0; i < count; i++) {
    if (i > 0) {
      json += ",";
    }
    bool openNetwork = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    json += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\"" +
            ",\"rssi\":" + String(WiFi.RSSI(i)) +
            ",\"secure\":" + jsonBool(!openNetwork) +
            ",\"security\":\"" + String(openNetwork ? "OPEN" : "SECURED") + "\"" +
            "}";
  }
  json += "]}";
  WiFi.scanDelete();
  _scanStarted = false;
  setStaStateFromWiFiStatus();
  _server.send(200,
               "application/json",
               json);
}

int WebDashboard::calculateSignalQuality(int rssi) const {
  if (rssi <= -100) {
    return 0;
  }
  if (rssi >= -50) {
    return 100;
  }
  return 2 * (rssi + 100);
}

void WebDashboard::addCoordinatorCors() {
  _server.sendHeader("Access-Control-Allow-Origin", WiFiConfig::COORDINATOR_ORIGIN);
  _server.sendHeader("Vary", "Origin");
}

bool WebDashboard::controlOriginAllowed() {
  if (!_server.hasHeader("Origin")) {
    return true;
  }

  const String origin = _server.header("Origin");
  if (origin.length() == 0) {
    return true;
  }
  if (origin == WiFiConfig::COORDINATOR_ORIGIN) {
    addCoordinatorCors();
    return true;
  }
  const String softApIp=WiFi.softAPIP().toString();
  const bool staConnected=WiFi.status() == WL_CONNECTED;
  const String staIp=staConnected ? WiFi.localIP().toString() : "";
  if (robotControlOriginAllowed(origin.c_str(), softApIp.c_str(),
                                staIp.c_str(), staConnected)) return true;

  ++_httpErrorCount;
  _server.send(403, "application/json", "{\"ok\":false,\"error\":\"origin_not_allowed\"}");
  return false;
}

bool WebDashboard::wifiProvisioningAllowed() {
  return _data.controlMode == "MANUAL" &&
         _data.motorState == "STOP" &&
         (_pendingCommand.length() == 0 || _pendingCommand == "STOP");
}

void WebDashboard::prepareControlResponse() {
  WiFiClient client = _server.client();
  client.setNoDelay(true);
  client.setTimeout(RobotConfig::HTTP_CLIENT_TIMEOUT_SECONDS);
}

void WebDashboard::sendError(int status, const char* contentType, const char* body) {
  ++_httpErrorCount;
  _server.send(status, contentType, body);
}

void WebDashboard::sendOk() {
  _server.send(200, "text/plain", "OK");
}

void WebDashboard::sendJson() {
  String json;
  json.reserve(6144);
  json = String("{\"node_id\":\"") + RobotConfig::NODE_ID + "\"" +
         ",\"telemetry_age_ms\":" + String(millis() - _data.uptimeMs) +
         ",\"product\":\"" + _data.product + "\"" +
         ",\"firmware\":\"" + _data.firmware + "\"" +
         ",\"stage\":\"" + _data.stage + "\"" +
         ",\"stage2Status\":\"" + _data.stage2Status + "\"" +
         ",\"uptimeMs\":" + String(_data.uptimeMs) +
         ",\"mode\":\"" + _data.controlMode + "\"" +
         ",\"turnMode\":\"" + _data.turnMode + "\"" +
         ",\"distance\":" + jsonFloat(_data.frontDistance, 1) +
         ",\"distanceValid\":" + jsonBool(_data.distanceValid) +
         ",\"ultrasonicState\":\"" + _data.ultrasonicState + "\"" +
         ",\"distanceSampleAgeMs\":" + String(_data.distanceSampleAgeMs) +
         ",\"distanceSampleSequence\":" + String(_data.distanceSampleSequence) +
         ",\"ultrasonicInvalidCount\":" + String(_data.ultrasonicInvalidCount) +
         ",\"ultrasonicFaultCount\":" + String(_data.ultrasonicFaultCount) +
         ",\"servoAngle\":" + String(_data.servoAngle) +
         ",\"scanOrientation\":\"" + _data.scanOrientation + "\"" +
         ",\"manualInspectionObstacle\":" + jsonBool(_data.manualInspectionObstacle) +
         ",\"manualInspectionOrientation\":\"" + _data.manualInspectionOrientation + "\"" +
         ",\"temperature\":" + jsonFloat(_data.temperature, 1) +
         ",\"temperature_valid\":" + jsonBool(_data.temperatureValid) +
         ",\"humidity\":" + jsonFloat(_data.humidity, 1) +
         ",\"humidity_valid\":" + jsonBool(_data.humidityValid) +
         ",\"environment_sample_age_ms\":" + String(_data.environmentSampleAgeMs) +
         ",\"dht_error_count\":" + String(_data.dhtErrorCount) +
         ",\"gasValue\":" + String(_data.gasValue) +
         ",\"gasFilteredValue\":" + String(_data.gasFilteredValue) +
         ",\"mq2Warmup\":" + jsonBool(_data.mq2Warmup) +
         ",\"mq2_sample_age_ms\":" + String(_data.mq2SampleAgeMs) +
         ",\"mq2_warmup_age_ms\":" + String(_data.mq2WarmupAgeMs) +
         ",\"gasStatus\":\"" + _data.gasStatus + "\"" +
         ",\"flameDetected\":" + jsonBool(_data.flameDetected) +
         ",\"gasCritical\":" + jsonBool(_data.gasCritical) +
         ",\"flameCritical\":" + jsonBool(_data.flameCritical) +
         ",\"temperatureCritical\":" + jsonBool(_data.temperatureCritical) +
         ",\"flame_state\":\"" + _data.flameState + "\"" +
         ",\"flame_input\":\"DIGITAL_DO\"" +
         ",\"flame_digital_level\":" + String(_data.flameDigitalLevel) +
         ",\"flame_active_level\":\"HIGH\"" +
         ",\"flame_raw_active\":" + jsonBool(_data.flameRawActive) +
         ",\"flame_sample_valid\":" + jsonBool(_data.flameSampleValid) +
         ",\"flame_detect_persist_ms\":" + String(RobotConfig::FLAME_DETECT_PERSIST_MS) +
         ",\"flame_clear_persist_ms\":" + String(RobotConfig::FLAME_CLEAR_PERSIST_MS) +
         ",\"flame_sample_age_ms\":" + String(_data.flameSampleAgeMs) +
         ",\"left_ir_raw\":" + String(_data.leftIrRaw) +
         ",\"right_ir_raw\":" + String(_data.rightIrRaw) +
         ",\"left_obstacle\":" + jsonBool(_data.leftObstacle) +
         ",\"right_obstacle\":" + jsonBool(_data.rightObstacle) +
         ",\"irObstacleDetected\":" + jsonBool(_data.irObstacleDetected) +
         ",\"nearObstacleHazard\":" + jsonBool(_data.nearObstacleHazard) +
         ",\"nearObstacleSource\":\"" + _data.nearObstacleSource + "\"" +
         ",\"ultrasonicWarning\":" + jsonBool(_data.ultrasonicWarning) +
         ",\"safetyState\":\"" + _data.safetyState + "\"" +
         ",\"alarmReason\":\"" + _data.alarmReason + "\"" +
         ",\"alarmSoundState\":\"" + _data.alarmSoundState + "\"" +
         ",\"buzzerState\":\"" + _data.buzzerState + "\"" +
         ",\"motorState\":\"" + _data.motorState + "\"" +
         ",\"speed_percent\":" + String(_data.speedPercent) +
         ",\"obstacleDecision\":\"" + _data.obstacleDecision + "\"" +
         ",\"navigationDecision\":\"" + _data.navigationDecision + "\"" +
         ",\"autoNavigation\":{" +
           "\"state\":\"" + _data.autoNavigationState + "\"" +
           ",\"ir_zone\":\"" + _data.autoIrZoneState + "\"" +
           ",\"avoidance_source\":\"" + _data.autoAvoidanceSource + "\"" +
           ",\"preferred_path\":\"" + _data.autoPreferredPath + "\"" +
           ",\"recovery_attempts\":" + String(_data.autoObstacleRecoveryAttempts) +
           ",\"blocked_retry_count\":" + String(_data.autoBlockedRetryCount) +
           ",\"turn_target_deg\":" + jsonFloat(_data.autoTurnTargetDeg, 1) +
           ",\"relative_turn_deg\":" + jsonFloat(_data.autoRelativeTurnDeg, 1) +
           ",\"selected_path\":\"" + _data.autoSelectedPath + "\"" +
           ",\"turn_completion\":\"" + _data.autoTurnCompletionReason + "\"" +
           ",\"scan\":{" +
             "\"left\":{\"state\":\"" + _data.autoScanLeftState + "\",\"distance_cm\":" + jsonFloat(_data.autoScanLeftDistanceCm, 1) + ",\"trusted\":" + jsonBool(_data.autoScanLeftTrusted) + "}" +
             ",\"front\":{\"state\":\"" + _data.autoScanFrontState + "\",\"distance_cm\":" + jsonFloat(_data.autoScanFrontDistanceCm, 1) + ",\"trusted\":" + jsonBool(_data.autoScanFrontTrusted) + "}" +
             ",\"right\":{\"state\":\"" + _data.autoScanRightState + "\",\"distance_cm\":" + jsonFloat(_data.autoScanRightDistanceCm, 1) + ",\"trusted\":" + jsonBool(_data.autoScanRightTrusted) + "}}" +
           ",\"hazard\":{\"state\":\"" + _data.autoHazardRecoveryState + "\",\"clear_elapsed_ms\":" + String(_data.autoHazardClearElapsedMs) + ",\"clear_remaining_ms\":" + String(_data.autoHazardClearRemainingMs) + "}" +
           ",\"stuck\":{\"state\":\"" + _data.autoStuckState + "\",\"attempts\":" + String(_data.autoStuckRecoveryAttempts) + ",\"last_reason\":\"" + _data.autoStuckLastReason + "\"}" +
           ",\"v2\":{\"state\":\"" + _data.autoV2State + "\",\"attempts\":" + String(_data.autoV2Attempts) + ",\"assistance_required\":" + jsonBool(_data.autoAssistanceRequired) + ",\"reason\":\"" + _data.autoV2Reason + "\"}" +
           ",\"episode\":{\"active\":" + jsonBool(_data.autoRecoveryEpisodeActive) +
             ",\"started_ms\":" + String(_data.autoRecoveryEpisodeStartedMs) +
             ",\"committed_side\":\"" + _data.autoRecoveryCommittedSide + "\",\"turns_used\":" + String(_data.autoRecoveryTurnsUsed) +
             ",\"turns_max\":" + String(_data.autoRecoveryTurnsMax) + ",\"ir_pulses\":" + String(_data.autoRecoveryIrPulses) +
             ",\"ir_active\":" + jsonBool(_data.autoRecoveryIrActive) + ",\"sonar_hold\":" + jsonBool(_data.autoSonarHold) +
             ",\"last_transition\":\"" + _data.autoRecoveryTransition + "\"}}" +
         ",\"commandSequence\":" + String(_lastCommandSequence) +
         ",\"softApIp\":\"" + _data.softApIp + "\"" +
         ",\"localHostname\":\"" + _data.localHostname + "\"" +
         ",\"mdnsActive\":" + jsonBool(_data.mdnsActive) +
         ",\"staConfigured\":" + jsonBool(_data.staConfigured) +
         ",\"staSsid\":\"" + jsonEscape(_data.staSsid) + "\"" +
         ",\"staStatus\":\"" + _data.staStatus + "\"" +
         ",\"staConnected\":" + jsonBool(_data.staConnected) +
         ",\"staIp\":\"" + _data.staIp + "\"" +
         ",\"staRssi\":" + String(_data.staRssi) +
         ",\"staSignalQuality\":" + String(_data.staSignalQuality) +
         ",\"imu\":{" +
           "\"initialized\":" + jsonBool(_data.imuInitialized) +
           ",\"healthy\":" + jsonBool(_data.imuHealthy) +
           ",\"health_state\":\"" + _data.imuHealthState + "\"" +
           ",\"error_count\":" + String(_data.imuErrorCount) +
           ",\"consecutive_failures\":" + String(_data.imuConsecutiveFailures) +
           ",\"accel_x\":" + jsonFloat(_data.accelX, 3) +
           ",\"accel_y\":" + jsonFloat(_data.accelY, 3) +
           ",\"accel_z\":" + jsonFloat(_data.accelZ, 3) +
           ",\"gyro_x\":" + jsonFloat(_data.gyroX, 3) +
           ",\"gyro_y\":" + jsonFloat(_data.gyroY, 3) +
           ",\"gyro_z\":" + jsonFloat(_data.gyroZ, 3) +
           ",\"roll\":" + jsonFloat(_data.roll, 1) +
           ",\"pitch\":" + jsonFloat(_data.pitch, 1) +
           ",\"temperature\":" + jsonFloat(_data.imuTemperature, 1) +
           ",\"motion\":\"" + _data.motionState + "\"" +
           ",\"sample_age_ms\":" + String(_data.imuSampleAgeMs) + "}" +
         ",\"radio\":{" +
           "\"initialized\":" + jsonBool(_data.radioInitialized) +
           ",\"chip_detected\":" + jsonBool(_data.radioChipDetected) +
           ",\"listening\":" + jsonBool(_data.radioListening) +
           ",\"hardware\":\"" + _data.radioHardware + "\"" +
           ",\"peer\":\"" + _data.radioPeer + "\"" +
           ",\"channel\":" + String(_data.radioChannel) +
           ",\"data_rate\":\"" + _data.radioDataRate + "\"" +
           ",\"pa_level\":\"" + _data.radioPaLevel + "\"" +
           ",\"tx_attempts\":" + String(_data.radioTxAttempts) +
           ",\"tx_success\":" + String(_data.radioTxSuccess) +
           ",\"tx_failure\":" + String(_data.radioTxFailure) +
           ",\"rx_packets\":" + String(_data.radioRxPackets) +
           ",\"last_tx_age_ms\":" + (_data.radioHasTx ? String(_data.radioLastTxAgeMs) : "null") +
           ",\"last_rx_age_ms\":" + (_data.radioHasRx ? String(_data.radioLastRxAgeMs) : "null") + "}" +
         ",\"display\":{\"initialized\":" + jsonBool(_data.displayInitialized) + "}" +
         ",\"capabilities\":{\"mobile_drive\":true,\"manual_control\":true,\"autonomous_navigation\":true,\"ultrasonic\":true,\"dual_ir\":true,\"environmental_sensing\":true,\"imu\":true,\"wifi\":true,\"nrf24\":true,\"local_oled\":true}" +
         "}";
  json.remove(json.length() - 1);
  auto appendKey = [&json](const char* key) {
    json += ',';
    json += char(34);
    json += key;
    json += char(34);
    json += ':';
  };
  appendKey("lastAcceptedCommand"); json += char(34); json += jsonEscape(_lastAcceptedCommand); json += char(34);
  appendKey("lastAppliedCommand"); json += char(34); json += jsonEscape(_lastAppliedCommand); json += char(34);
  appendKey("lastStopReason"); json += char(34); json += jsonEscape(_lastStopReason); json += char(34);
  appendKey("manualFlameGasOverride"); json += jsonBool(_data.manualFlameGasOverride);
  appendKey("manualMotionInhibitReason"); json += char(34); json += jsonEscape(_data.manualMotionInhibitReason); json += char(34);
  appendKey("controlSessionAlive"); json += jsonBool(sessionAlive());
  appendKey("commandAcceptedMs"); json += String(_lastCommandAcceptedMs);
  appendKey("commandAppliedMs"); json += String(_lastCommandAppliedMs);
  appendKey("commandAgeMs"); json += _lastCommandAcceptedMs == 0 ? "null" : String(millis() - _lastCommandAcceptedMs);
  appendKey("handlerApplyLagMs"); json += _lastCommandAppliedMs == 0 || _lastCommandAppliedMs < _lastCommandAcceptedMs ? "null" : String(_lastCommandAppliedMs - _lastCommandAcceptedMs);
  appendKey("httpErrorCount"); json += String(_httpErrorCount);
  appendKey("httpTimeoutCount"); json += String(_server.timeoutCount());
  appendKey("wifiReconnectCount"); json += String(_wifiReconnectCount);
  appendKey("wifiStaDisconnectCount"); json += String(_data.wifiStaDisconnectCount);
  appendKey("wifiApTransitionCount"); json += String(_data.wifiApTransitionCount);
  appendKey("wifiLastDisconnectReason"); json += String(_data.wifiLastDisconnectReason);
  appendKey("wifiLastEvent"); json += String(_data.wifiLastEvent);
  appendKey("softApChannel"); json += String(_data.softApChannel);
  appendKey("softApClients"); json += String(_data.softApClients);
  appendKey("radioEnabled"); json += jsonBool(_data.radioEnabled);
  appendKey("freeHeap"); json += String(_data.freeHeap);
  appendKey("minimumFreeHeap"); json += String(_data.minimumFreeHeap);
  appendKey("psramSize"); json += String(_data.psramSize);
  appendKey("resetReason"); json += char(34); json += jsonEscape(_data.resetReason); json += char(34);
  appendKey("maximumServiceLoopGapMs"); json += String(_data.maximumServiceLoopGapMs);
  json += '}';
  _server.send(200, "application/json", json);
}
