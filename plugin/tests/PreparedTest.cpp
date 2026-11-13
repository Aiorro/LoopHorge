#include "../Source/engine/Prepared.h"
#include "AllocationGuard.h"
#include <cstdlib>
#include <iostream>
#include <atomic>
#include <limits>
using namespace composer;
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
Composition fixture(){
    Composition c;c.id="prepared";c.endTick={1920};c.tempos={{{0},500000},{{480},250000}};c.meters={{{0},4,4}};c.keys={{{0},0,Scale::major}};
    Track t;t.id=1;t.name="Track";t.channel=16;t.program=33;t.bank=Bank{2,7};
    t.notes={{2,60,90,{0},{480}},{3,60,100,{240},{720}},{4,60,110,{240},{240}},{5,60,85,{960},{480}}};
    t.controls={{20,{0},64,127},{21,{1200},1,20},{22,{600},1,10},{23,{600},1,15}};c.tracks={t};return c;
}
int main(){
    auto c=fixture();auto ready=prepareComposition(c);if(!ready.song)std::cerr<<ready.error;check(ready.song);const auto& s=*ready.song;
    check(s.endTick().value==1920&&s.durationSeconds()==1.25&&s.storageBytes()>0);
    auto state=s.chase({239});check(state.noteCount==1&&state.notes[0].key==15*128+60&&state.notes[0].count==1&&state.notes[0].velocity==90);
    state=s.chase({240});check(state.notes[0].count==3&&state.notes[0].velocity==110);
    state=s.chase({480});check(state.notes[0].count==1&&state.notes[0].velocity==110);
    state=s.chase({960});check(state.noteCount==1&&state.notes[0].count==1&&state.notes[0].velocity==85);
    state=s.chase({1440});check(state.noteCount==0&&state.controlCount==2);
    state=s.chase({600});check(state.presets[15].program==33&&state.presets[15].bankMsb==2&&state.presets[15].bankLsb==7);
    check(state.controlCount==2);bool cc=false;for(size_t i=0;i<state.controlCount;++i)if(state.controls[i].controller==1)cc=state.controls[i].value==15;check(cc);
    check(s.chase({-1}).noteCount==0&&s.chase({1920}).controlCount==0);
    check(s.firstAfter({-1})==0&&s.firstAfter({1920})==s.eventCount()&&!s.event(s.eventCount()));
    auto first=s.firstAfter({240});check(s.event(first)&&s.event(first)->tick.value==480);
    check(s.secondsAtTick(480)==0.5&&s.secondsAtTick(960)==0.75&&s.tickAtSeconds(0.75)==960.0&&s.tickAtSeconds(1.25)==1920.0);
    check(!s.secondsAtTick(-1)&&!s.tickAtSeconds(2)&&!s.tickAtSeconds(std::numeric_limits<double>::quiet_NaN()));
    std::atomic<bool> cancelled{true};check(!prepareComposition(c,&cancelled).song);cancelled=false;check(prepareComposition(c,&cancelled).song);
    auto bad=c;bad.tracks[0].notes[0].velocity=128;check(!prepareComposition(bad).song);
    auto budget=c;budget.tracks[0].notes.reserve((limits::documentBytes-8192)/sizeof(Note));check(validate(budget));check(!prepareComposition(budget).song);
    auto dense=c;dense.tracks[0].notes.clear();dense.tracks[0].controls.clear();
    for(size_t i=0;i<131072;++i)dense.tracks[0].notes.push_back({i+10,60,90,{0},{480}});
    auto d=prepareComposition(dense);check(d.song);check(d.song->chase({0}).notes[0].count==131072&&d.song->chase({480}).noteCount==0&&d.song->eventCount()==2);
    auto maximum=c;maximum.tracks.clear();
    for(int ch=1;ch<=16;++ch){Track t;t.id=Id(ch);t.name="Channel";t.channel=ch;t.role=ch==10?Role::drums:Role::bass;t.bank=Bank{1,2};t.program=3;
        for(int i=0;i<16;++i){t.notes.push_back({Id(1000+ch*16+i),i,90,{0},{480}});t.controls.push_back({Id(3000+ch*16+i),{0},i+1,64});}maximum.tracks.push_back(t);}
    auto m=prepareComposition(maximum);check(m.song);state=m.song->chase({0});check(state.noteCount==256&&state.controlCount==256);
    for(const auto& p:state.presets)check(p.program==3&&p.bankMsb==1&&p.bankLsb==2);
    maximum.tracks[0].notes.push_back({9999,100,90,{0},{480}});check(!prepareComposition(maximum).song);
    c=fixture();c.tempos.clear();c.endTick={8192};for(int i=0;i<4096;++i)c.tempos.push_back({{i*2},100000+i});
    auto many=prepareComposition(c);check(many.song);for(int i=0;i<=8192;++i){auto secs=many.song->secondsAtTick(i);check(secs);check(std::abs(*many.song->tickAtSeconds(*secs)-i)<0.00001);}
    allocationGuard::allocations=allocationGuard::frees=0;allocationGuard::active=true;
    uint64_t observed=0;for(int i=0;i<8192;++i){auto query=many.song->chase({i});observed+=query.noteCount;auto time=many.song->secondsAtTick(i);observed+=uint64_t(*many.song->tickAtSeconds(*time));observed+=many.song->firstAfter({i});}
    allocationGuard::active=false;check(observed&&allocationGuard::allocations==0&&allocationGuard::frees==0);
    check(c.tracks[0].notes[0].id==2&&c.tracks[0].notes[0].duration.value==480);
    std::cout<<"prepared indexed note counts/CC/presets, maximum lifetimes/density, tempo conversions and cancellation passed\n";
}
