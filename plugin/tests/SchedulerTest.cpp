#include "../Source/Schedule.h"
#include <cstdlib>
#include <iostream>
#include <vector>
void check(bool value) { if (!value) std::abort(); }
int main() {
 Schedule s;
 check(s.add(60, 0, 1, 100, 1));
 check(!s.add(128, 0, 1, 100, 1));
 check(!s.add(60, -1, 1, 100, 1));
 check(!s.add(60, 15.5, 1, 100, 1));
 check(!s.add(60, 16, 1e-300, 100, 1));
 check(s.add(60, 1, 1, 100, 1));
 s.sort();
 check(s.count==4);
 check(s.events[1].beat==1 && !s.events[1].on);
 check(s.events[2].on);
 for(int i=0;i<1598;++i) check(s.add(40,0,1,80,2));
 check(!s.add(40,0,1,80,2));
 Schedule varied;
 for(int i=0;i<1600;++i)check(varied.add(i%128,((i*7919)%15800)*0.001,0.1,100,1+i%3));
 s=varied;s.sort();
 for(int i=0;i<10000;++i){double local=((i*7919)%16000)*0.001,step=0.000137;
  std::vector<const Event*> expected,actual;
  for(int n=0;n<s.count;++n){auto& e=s.events[n];if(e.beat>=s.beats)continue;double delta=local-e.beat;if(delta< -1e-9)delta+=s.beats;if(delta>=-1e-9&&delta<step-1e-9)expected.push_back(&e);}
  s.forEventsAt(local,step,[&](const Event& e){actual.push_back(&e);});check(expected==actual);
 }
 std::cout<<"schedule validation, ordering, capacity passed\n";
}
