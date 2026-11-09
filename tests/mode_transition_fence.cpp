#include "ModeTransitionFence.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
  ModeTransitionFence f; f.reset(123); f.issue(100);
  char ticket[32]; std::snprintf(ticket,sizeof(ticket),"%u.%u",f.boot(),f.revision());
  assert(f.consume(ticket,101)); assert(!f.consume(ticket,102));
  f.issue(200); std::snprintf(ticket,sizeof(ticket),"%u.%u",f.boot(),f.revision());
  f.revoke(); assert(!f.consume(ticket,201)); // later STOP wins over old AUTO
  f.issue(300); std::snprintf(ticket,sizeof(ticket),"%u.%u",f.boot(),f.revision());
  assert(!f.consume(ticket,2300)); // exact expiry is rejected
  f.issue(0xfffffff0U); std::snprintf(ticket,sizeof(ticket),"%u.%u",f.boot(),f.revision());
  assert(f.consume(ticket,20)); // unsigned elapsed time across wrap
  f.issue(0); std::snprintf(ticket,sizeof(ticket),"%u.%u",f.boot(),f.revision());
  f.reset(124); assert(!f.consume(ticket,1)); // reboot invalidates prior boot
  for(const char* bad : {"", "1", "-1.1", "1.-1", "4294967296.1", "1.4294967296", "1.1x", "1.1.1", "00000000001.1"})
    assert(!f.consume(bad,1));
}
