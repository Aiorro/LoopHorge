#include "../Source/engine/Playback.h"
#include "AllocationGuard.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace composer;
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
struct Capture {
    struct Item {MidiPacket midi;int sample;};std::array<Item,2048> items{};size_t count=0;
    void operator()(MidiPacket p,int sample)noexcept{if(count==items.size())std::abort();items[count++]={p,sample};}
    void clear(){count=0;}
    int matching(int status,int pitch=-1,int sample=-1)const{int n=0;for(size_t i=0;i<count;++i){const auto& x=items[i];if((x.midi.status&0xf0)==status&&(pitch<0||x.midi.data1==pitch)&&(sample<0||x.sample==sample))++n;}return n;}
};
Composition fixture(){
    Composition c;c.id="playback";c.endTick={1920};c.tempos={{{0},500000},{{480},250000}};c.meters={{{0},4,4}};c.keys={{{0},0,Scale::major}};
    Track t;t.id=1;t.name="Channel";t.channel=16;t.notes={{2,60,90,{0},{480}},{3,60,100,{240},{720}},{4,60,110,{240},{240}},{5,60,85,{960},{480}}};c.tracks={t};return c;
}
PreparedSong ready(const Composition& c){auto r=prepareComposition(c);check(r.song);return std::move(*r.song);}
int main(){
    auto s=ready(fixture());Playback player;Capture out;PlaybackBlock b;b.samples=13000;
    auto result=player.render(s,b,out);check(!result.overflow&&!result.invalidClock&&result.messages==3);
    check(out.matching(0x90,60,0)==1&&out.matching(0x80,60,12000)==1&&out.matching(0x90,60,12000)==1);check(out.items[2].midi.data2==110);
    b.position=result.nextPosition;b.samples=12000;out.clear();result=player.render(s,b,out);check(out.count==0); // earlier ends do not release the retrigger
    b.position=2;out.clear();result=player.render(s,b,out);check(out.matching(0x90,60,0)==1&&out.items[out.count-1].midi.data2==85); // seek boundary is chased once
    b.playing=false;out.clear();result=player.render(s,b,out);check(out.matching(0x80,60,0)==1&&result.messages==4);
    b.playing=true;b.position=3;out.clear();result=player.render(s,b,out);check(out.matching(0x90)==0); // pedal history does not resurrect ended notes
    {Playback p;PlaybackBlock block;block.position=0.5;block.samples=13000;out.clear();auto r=p.render(s,block,out);check(r.messages==1&&out.items[0].midi.data2==110);block.position=r.nextPosition;block.samples=48000;out.clear();p.render(s,block,out);check(out.matching(0x80,60,23000)==1&&out.matching(0x90,60,23000)==1&&out.matching(0x80,60,47000)==1);}
    {auto changed=fixture();changed.tracks[0].channel=1;changed.tracks[0].notes={{2,64,80,{0},{480}}};auto replacement=ready(changed);Playback p;PlaybackBlock block;out.clear();auto r=p.render(s,block,out);block.position=r.nextPosition;out.clear();p.render(replacement,block,out);check(out.items[0].midi.status==0x8f&&out.items[0].midi.data1==60&&out.items[out.count-1].midi.status==0x90&&out.items[out.count-1].midi.data1==64);}
    // Continuous small blocks deliver the first sample at/after a fractional onset.
    auto tiny=fixture();tiny.tracks[0].notes={{2,60,90,{1},{1}}};auto ts=ready(tiny);
    for(int blockSize:{32,64}){
        Playback p;PlaybackBlock block;block.samples=blockSize;block.sampleRate=44100;int on=-1,off=-1;
        for(int n=0;n<5;++n){out.clear();auto r=p.render(ts,block,out);for(size_t i=0;i<out.count;++i){auto& x=out.items[i];if((x.midi.status&0xf0)==0x90)on=n*blockSize+x.sample;if((x.midi.status&0xf0)==0x80)off=n*blockSize+x.sample;}block.position=r.nextPosition;}
        check(on==46&&off==92);
    }
    {Playback p;PlaybackBlock block;block.samples=50;out.clear();auto r=p.render(ts,block,out);check(out.count==0);block.position=r.nextPosition;out.clear();r=p.render(ts,block,out);check(out.matching(0x90,60,0)==1);block.position=r.nextPosition;out.clear();p.render(ts,block,out);check(out.matching(0x80,60,0)==1);}
    {auto ending=fixture();ending.tracks[0].notes={{2,60,90,{0},{1920}}};auto endSong=ready(ending);Playback p;PlaybackBlock block;block.samples=96000;out.clear();auto r=p.render(endSong,block,out);check(!r.ended&&out.matching(0x80)==0);block.position=r.nextPosition;block.samples=32;out.clear();r=p.render(endSong,block,out);check(r.ended&&r.messages==4&&out.matching(0x80,60,0)==1);block.position=r.nextPosition;out.clear();check(p.render(endSong,block,out).messages==0);}
    {auto control=fixture();control.tracks[0].controls={{20,{0},101,0},{21,{0},100,0},{22,{0},6,2},{23,{1},101,1},{24,{1},100,2},{25,{1},6,3}};auto cs=ready(control);Playback p;PlaybackBlock block;block.samples=64;out.clear();p.render(cs,block,out);check(out.items[0].midi.data1==101&&out.items[1].midi.data1==100&&out.items[2].midi.data1==6);check(out.items[4].midi.data1==101&&out.items[5].midi.data1==100&&out.items[6].midi.data1==6);}
    // Standalone follows all tempo segments; host timing uses its own BPM.
    auto timed=fixture();timed.tempos.push_back({{960},125000});timed.tracks[0].notes={{2,60,90,{480},{480}},{3,62,90,{960},{480}}};auto tempoSong=ready(timed);
    {Playback p;PlaybackBlock block;block.clock=Clock::compositionSeconds;block.samples=60000;out.clear();auto r=p.render(tempoSong,block,out);check(r.ended&&r.nextPosition==1.0);check(out.matching(0x90,60,24000)==1&&out.matching(0x80,60,36000)==1&&out.matching(0x90,62,36000)==1&&out.matching(0x80,62,42000)==1);}
    {Playback p;PlaybackBlock block;block.samples=60000;out.clear();auto r=p.render(tempoSong,block,out);check(!r.ended&&out.matching(0x90,62,48000)==1&&out.matching(0x80,62)==0);}
    // Origin is absolute and may be crossed within a block, without a play reanchor.
    {Playback p;PlaybackBlock block;block.position=3.999;block.hostStartQuarterBeat=4;block.samples=64;out.clear();p.render(s,block,out);check(out.matching(0x90,60,24)==1);}
    {Playback p;PlaybackBlock block;block.position=-1;block.samples=32;out.clear();p.render(s,block,out);check(out.count==0);block.position=0;out.clear();auto r=p.render(s,block,out);check(out.matching(0x90,60,0)==1);block.position=r.nextPosition;block.bpm=60;out.clear();p.render(s,block,out);check(out.count==0);}
    {Playback p;PlaybackBlock block;out.clear();auto r=p.render(s,block,out);block.position=r.nextPosition;block.sampleRate=0;out.clear();r=p.render(s,block,out);check(r.invalidClock&&out.matching(0x80,60)==1&&r.messages==4);block.sampleRate=48000;block.position=std::numeric_limits<double>::quiet_NaN();out.clear();check(p.render(s,block,out).invalidClock);}
    {Playback p;PlaybackBlock block;out.clear();p.render(s,block,out);block.samples=0;block.playing=false;out.clear();check(p.render(s,block,out).messages==0);block.samples=32;check(p.render(s,block,out).messages==4);}
    auto max=fixture();max.tracks.clear();
    for(int ch=1;ch<=16;++ch){Track t;t.id=Id(ch);t.name="Channel";t.channel=ch;t.role=ch==10?Role::drums:Role::bass;t.bank=Bank{1,2};t.program=3;for(int i=0;i<16;++i){t.notes.push_back({Id(1000+ch*16+i),i,90,{0},{480}});t.controls.push_back({Id(3000+ch*16+i),{0},i+1,64});}max.tracks.push_back(t);}
    auto maximum=ready(max);
    {Playback p;PlaybackBlock block;out.clear();auto r=p.render(maximum,block,out);check(r.messages==560&&out.matching(0x90)==256);block.playing=false;out.clear();r=p.render(maximum,block,out);check(r.messages==304&&out.matching(0x80)==256);}
    auto overflow=fixture();overflow.endTick={131073};overflow.tracks[0].notes.clear();for(size_t i=0;i<131072;++i)overflow.tracks[0].notes.push_back({i+10,60,90,{0},{int64_t(i+1)}});auto work=ready(overflow);
    {Playback p;PlaybackBlock block;block.samples=10000000;out.clear();auto r=p.render(work,block,out);check(r.overflow&&r.events==1664&&r.messages<=2048&&out.matching(0x80)==1);block.position=r.nextPosition;out.clear();r=p.render(work,block,out);check(r.overflow&&r.messages==0);block.playing=false;out.clear();p.render(work,block,out);block.playing=true;block.position=0;block.samples=32;out.clear();check(!p.render(work,block,out).overflow&&out.matching(0x90)==1);}
    auto crowded=fixture();crowded.endTick={7680};crowded.tracks[0].notes={{2,60,90,{0},{7680}}};for(int i=1;i<=2000;++i)crowded.tracks[0].controls.push_back({Id(10000+i),{i},1,i%128});auto messages=ready(crowded);
    {Playback p;PlaybackBlock block;block.samples=200000;out.clear();auto r=p.render(messages,block,out);check(r.overflow&&r.messages<=2048&&out.matching(0x90)==out.matching(0x80));}
    // Track actual cold/seek/steady/stop/overflow rendering with a fixed-capacity sink.
    allocationGuard::allocations=allocationGuard::frees=0;allocationGuard::active=true;
    {Playback p;PlaybackBlock block;block.samples=32;out.clear();auto r=p.render(maximum,block,out);block.position=r.nextPosition;out.clear();p.render(maximum,block,out);block.position=0.5;out.clear();p.render(maximum,block,out);block.playing=false;out.clear();p.render(maximum,block,out);block.playing=true;block.position=0;block.samples=10000000;out.clear();p.render(work,block,out);out.clear();p.render(messages,block,out);}
    allocationGuard::active=false;check(allocationGuard::allocations==0&&allocationGuard::frees==0);
    std::cout<<"bounded MIDI rendering: overlap/seek/origin/tempo/32-64buffers,256keys, persistent overflow and no new/delete passed\n";
}
