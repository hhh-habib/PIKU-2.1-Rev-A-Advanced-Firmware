#include "PriorityWebServer.h"

#include "RobotConfig.h"
#include <errno.h>
#include <sys/socket.h>

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
          if (millis() - _statusChange <= RobotConfig::HTTP_REQUEST_PARSE_TIMEOUT_MS) {
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

void PriorityWebServer::beginTelemetryResponse() {
  telemetryResponse_ = true;
  telemetryStartedMs_ = 0;
}

void PriorityWebServer::endTelemetryResponse() {
  telemetryResponse_ = false;
}

size_t PriorityWebServer::_currentClientWrite(const char* data, size_t length) {
  if (!telemetryResponse_) return WebServer::_currentClientWrite(data, length);
  // The synchronous server cannot accept STOP while writing /data. Never
  // wait for a stalled telemetry socket's multi-second WiFiClient retry loop.
  if (telemetryStartedMs_ == 0) telemetryStartedMs_ = millis();
  const int socketFd = _currentClient.fd();
  if (socketFd < 0) return 0;
  size_t sent = 0;
  while (sent < length &&
         millis() - telemetryStartedMs_ < RobotConfig::HTTP_TELEMETRY_SEND_BUDGET_MS) {
    const int result = ::send(socketFd, data + sent, length - sent, MSG_DONTWAIT);
    if (result > 0) {
      sent += static_cast<size_t>(result);
    } else if (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      break;
    } else {
      yield();
    }
  }
  if (sent != length) {
    ++timeoutCount_;
    _currentClient.stop();
  }
  return sent;
}
