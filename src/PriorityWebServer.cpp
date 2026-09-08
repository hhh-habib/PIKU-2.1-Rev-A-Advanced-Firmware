#include "PriorityWebServer.h"

#include "RobotConfig.h"

void PriorityWebServer::handleClient() {
  if (_currentStatus == HC_NONE) {
    _currentClient = _server.available();
    if (!_currentClient) {
      if (_nullDelay) delay(1);
      return;
    }

    // WiFiClient::setTimeout is seconds-based in this framework version, but
    // HTTP parsing uses Stream reads. Qualify the base call for millisecond
    // precision before _parseRequest can wait on an incomplete request.
    static_cast<Stream&>(_currentClient).setTimeout(
      RobotConfig::HTTP_REQUEST_PARSE_TIMEOUT_MS);
    _currentClient.setNoDelay(true);
    _currentStatus = HC_WAIT_READ;
    _statusChange = millis();
  }

  bool keepCurrentClient = false;
  bool callYield = false;
  if (_currentClient.connected()) {
    switch (_currentStatus) {
      case HC_NONE:
        break;
      case HC_WAIT_READ:
        if (_currentClient.available()) {
          const unsigned long parseStartedMs = millis();
          if (_parseRequest(_currentClient)) {
            _currentClient.setTimeout(HTTP_MAX_SEND_WAIT / 1000);
            _contentLength = CONTENT_LENGTH_NOT_SET;
            _handleRequest();
          } else if (millis() - parseStartedMs >= RobotConfig::HTTP_REQUEST_PARSE_TIMEOUT_MS) {
            ++timeoutCount_;
          }
        } else {
          if (millis() - _statusChange <= HTTP_MAX_DATA_WAIT) {
            keepCurrentClient = true;
          } else {
            ++timeoutCount_;
          }
          callYield = true;
        }
        break;
      case HC_WAIT_CLOSE:
        if (millis() - _statusChange <= HTTP_MAX_CLOSE_WAIT) {
          keepCurrentClient = true;
          callYield = true;
        }
        break;
    }
  }

  if (!keepCurrentClient) {
    _currentClient = WiFiClient();
    _currentStatus = HC_NONE;
    _currentUpload.reset();
    _currentRaw.reset();
  }
  if (callYield) yield();
}
