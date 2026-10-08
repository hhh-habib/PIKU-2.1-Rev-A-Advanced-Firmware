#pragma once

#include <WebServer.h>

// Preserves Arduino WebServer routing while bounding the accepted client's
// Stream timeout before request-line/header parsing begins.
class PriorityWebServer final : public WebServer {
 public:
  explicit PriorityWebServer(int port = 80) : WebServer(port) {}
  void handleClient() override;
  void beginTelemetryResponse();
  void endTelemetryResponse();
  unsigned long timeoutCount() const { return timeoutCount_; }

 protected:
  size_t _currentClientWrite(const char* data, size_t length) override;

 private:
  bool telemetryResponse_ = false;
  unsigned long telemetryStartedMs_ = 0;
  unsigned long timeoutCount_ = 0;
};
