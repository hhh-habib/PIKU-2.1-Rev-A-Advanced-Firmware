#include "PriorityWebServer.h"

#include "RobotConfig.h"
#include <errno.h>
#include <sys/socket.h>
#include <cstring>
#include <strings.h>

bool PriorityWebServer::requestReady() {
  // No Stream read has consumed this newly accepted socket. Peek a bounded
  // header and wait across service passes until the entire framed request is
  // present; per-character Stream timeouts are not a whole-request deadline.
  char* header = requestHeader_;
  const int count = ::recv(_currentClient.fd(), header, sizeof(requestHeader_) - 1,
                           MSG_PEEK | MSG_DONTWAIT);
  if (count <= 0) return false;
  header[count] = '\0';
  const char* end = strstr(header, "\r\n\r\n");
  if (!end) return false;
  const size_t headerBytes = static_cast<size_t>(end - header) + 4;
  size_t bodyBytes = 0;
  bool hasLength = false;
  const char* line = strstr(header, "\r\n");
  if (!line) return false;
  line += 2;
  while (line < end) {
    const char* next = strstr(line, "\r\n");
    if (!next) return false;
    if (strncasecmp(line, "Transfer-Encoding:", 18) == 0) return false;
    if (strncasecmp(line, "Content-Length:", 15) == 0) {
      if (hasLength) return false;
      hasLength = true;
      const char* value = line + 15;
      while (value < next && (*value == ' ' || *value == '\t')) ++value;
      if (value == next || *value < '0' || *value > '9') return false;
      while (value < next && *value >= '0' && *value <= '9') {
        const size_t digit = static_cast<size_t>(*value++ - '0');
        if (bodyBytes > (RobotConfig::HTTP_MAX_BODY_BYTES - digit) / 10) return false;
        bodyBytes = bodyBytes * 10 + digit;
      }
      while (value < next && (*value == ' ' || *value == '\t')) ++value;
      if (value != next || bodyBytes > RobotConfig::HTTP_MAX_BODY_BYTES) return false;
    }
    line = next + 2;
  }
  return static_cast<size_t>(_currentClient.available()) >= headerBytes + bodyBytes;
}

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
        if (millis() - _statusChange >= RobotConfig::HTTP_REQUEST_PARSE_TIMEOUT_MS) {
          ++timeoutCount_;
          break;
        }
        if (_currentClient.available()) {
          if (!requestReady()) {
            keepCurrentClient = true;
            callYield = true;
            break;
          }
          const unsigned long parseStartedMs = millis();
          if (_parseRequest(_currentClient)) {
            _currentClient.setTimeout(HTTP_MAX_SEND_WAIT / 1000);
            _contentLength = CONTENT_LENGTH_NOT_SET;
            responseFailed_ = false;
            responseStartedMs_ = millis();
            responseBudgetMs_ = RobotConfig::HTTP_CONTROL_SEND_BUDGET_MS;
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
  responseBudgetMs_ = RobotConfig::HTTP_TELEMETRY_SEND_BUDGET_MS;
}

void PriorityWebServer::endTelemetryResponse() {
  // The response deadline is shared by header and body; never refill it.
}

size_t PriorityWebServer::_currentClientWrite_P(PGM_P data, size_t length) {
  // ESP32 flash strings are memory mapped. Do not delegate send_P to the
  // framework's unbounded retrying write_P path.
  responseBudgetMs_ = RobotConfig::HTTP_ASSET_SEND_BUDGET_MS;
  return _currentClientWrite(data, length);
}

size_t PriorityWebServer::_currentClientWrite(const char* data, size_t length) {
  if (responseFailed_) return 0;
  const int socketFd = _currentClient.fd();
  if (socketFd < 0) return 0;
  size_t sent = 0;
  while (sent < length &&
         millis() - responseStartedMs_ < responseBudgetMs_) {
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
    responseFailed_ = true;
    ++timeoutCount_;
    _currentClient.stop();
  }
  return sent;
}
