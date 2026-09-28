#ifndef CONTROL_ORIGIN_POLICY_H
#define CONTROL_ORIGIN_POLICY_H
#include <stdio.h>
#include <string.h>

// Accept only this robot's current HTTP endpoints. Empty Origin is used by
// non-browser robot API clients; coordinator origin is checked by the caller.
inline bool robotControlOriginAllowed(const char* origin,
                                      const char* softApIp,
                                      const char* staIp,
                                      bool staConnected) {
  if (!origin || !*origin) return true;
  char endpoint[64];
  const char* hosts[3]={softApIp, staConnected ? staIp : nullptr, "piku.local"};
  for (const char* host : hosts) {
    if (!host || !*host || strcmp(host,"0.0.0.0")==0) continue;
    const int count=snprintf(endpoint,sizeof(endpoint),"http://%s",host);
    if (count>0 && static_cast<size_t>(count)<sizeof(endpoint) &&
        strcmp(origin,endpoint)==0) return true;
  }
  return false;
}
#endif
