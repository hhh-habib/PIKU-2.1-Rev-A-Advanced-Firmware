#include <cassert>
#include "ControlOriginPolicy.h"
int main() {
  const char* ap="192.168.4.1";
  const char* sta="10.41.7.83"; // arbitrary DHCP assignment
  assert(robotControlOriginAllowed("http://192.168.4.1",ap,sta,true));
  assert(robotControlOriginAllowed("http://10.41.7.83",ap,sta,true));
  assert(robotControlOriginAllowed("http://piku.local",ap,sta,true));
  assert(robotControlOriginAllowed("",ap,sta,true)); // backend API client
  assert(!robotControlOriginAllowed("http://10.41.7.83",ap,sta,false));
  assert(!robotControlOriginAllowed("http://10.41.7.84",ap,sta,true));
  assert(!robotControlOriginAllowed("https://10.41.7.83",ap,sta,true));
  assert(!robotControlOriginAllowed("http://evil.local",ap,sta,true));
  assert(!robotControlOriginAllowed("http://10.41.7.83.evil",ap,sta,true));
  assert(robotControlOriginAllowed("http://172.22.0.5","172.22.0.5","",false));
}
