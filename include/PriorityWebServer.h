#pragma once

#include <WebServer.h>
#include "RobotConfig.h"

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
  size_t _currentClientWrite_P(PGM_P data, size_t length) override;

 private:
  bool requestReady();
  bool responseFailed_ = false;
  unsigned long responseStartedMs_ = 0;
  unsigned long responseBudgetMs_ = 0;
  unsigned long timeoutCount_ = 0;
  // One fixed buffer keeps the request peek off the time-critical loop stack.
  char requestHeader_[RobotConfig::HTTP_MAX_HEADER_BYTES + 1];
};
