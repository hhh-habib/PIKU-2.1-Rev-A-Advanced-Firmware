#include <cassert>
#include "TraceRing.h"
int main() {
  TraceRing<int,3> q;
  int v=0;
  assert(!q.pop(v));
  q.push(1); q.push(2); q.push(3);
  assert(q.size()==3 && q.dropped()==0);
  q.push(4);
  assert(q.size()==3 && q.dropped()==1);
  assert(q.pop(v) && v==2);
  assert(q.pop(v) && v==3);
  assert(q.pop(v) && v==4);
  assert(!q.pop(v));
  q.clear();
  assert(q.size()==0 && q.dropped()==0);
}
