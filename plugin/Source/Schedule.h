#pragma once
#include <array>
#include <algorithm>
#include <cmath>
struct Event { double beat=0; int pitch=0, velocity=0, channel=1; bool on=false; };
struct Schedule {
 std::array<Event,3200> events{};
 int count=0; double beats=16, bpm=100;
 bool add(int pitch,double start,double duration,int velocity,int channel) {
  if(count>3198 || pitch<0 || pitch>127 || velocity<1 || velocity>127 || channel<1 || channel>16
    || !std::isfinite(start) || !std::isfinite(duration) || start<0 || duration<=0 || start+duration>beats) return false;
  events[count++]={start,pitch,velocity,channel,true};
  events[count++]={start+duration,pitch,0,channel,false};return true;
 }
 void sort(){std::sort(events.begin(),events.begin()+count,[](const auto& a,const auto& b){return a.beat<b.beat || (a.beat==b.beat && a.on<b.on);});}
};
