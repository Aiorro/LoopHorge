#include "../Source/Schedule.h"
#include <cstdlib>
#include <iostream>
void check(bool value) { if (!value) std::abort(); }
int main() {
 Schedule s;
 check(s.add(60, 0, 1, 100, 1));
 check(!s.add(128, 0, 1, 100, 1));
 check(!s.add(60, -1, 1, 100, 1));
 check(!s.add(60, 15.5, 1, 100, 1));
 check(s.add(60, 1, 1, 100, 1));
 s.sort();
 check(s.count==4);
 check(s.events[1].beat==1 && !s.events[1].on);
 check(s.events[2].on);
 for(int i=0;i<1598;++i) check(s.add(40,0,1,80,2));
 check(!s.add(40,0,1,80,2));
 std::cout<<"schedule validation, ordering, capacity passed\n";
}
