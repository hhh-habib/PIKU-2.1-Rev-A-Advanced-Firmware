#pragma once

#include <WebServer.h>

// Preserves Arduino WebServer routing while bounding the accepted client's
// Stream timeout before request-line/header parsing begins.
class PriorityWebServer final : public WebServer {
 public:
  explicit PriorityWebServer(int port = 80) : WebServer(port) {}
  void handleClient() override;
  unsigned long timeoutCount() const { return timeoutCount_; }

 private:
  unsigned long timeoutCount_ = 0;
};
