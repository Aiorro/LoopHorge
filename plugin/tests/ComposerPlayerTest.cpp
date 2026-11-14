#include "../Source/ComposerPlayer.h"
#include "../Source/PreparedMidi.h"
#include "AllocationGuard.h"
#include "Vst3ConversionCheck.h"
#include <thread>
#include <iostream>
using namespace composer;
#define check(x) do{if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
#if LOOPFORGE_WRAP_HEAP
extern "C" void* __real_malloc(size_t);extern "C" void* __real_calloc(size_t,size_t);extern "C" void* __real_realloc(void*,size_t);extern "C" void __real_free(void*);
extern "C" void* __wrap_malloc(size_t n){if(allocationGuard::active)++allocationGuard::allocations;return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t width){if(allocationGuard::active)++allocationGuard::allocations;return __real_calloc(n,width);}
extern "C" void* __wrap_realloc(void* p,size_t n){if(allocationGuard::active)++allocationGuard::allocations;return __real_realloc(p,n);}
extern "C" void __wrap_free(void* p){if(p&&allocationGuard::active)++allocationGuard::frees;__real_free(p);}
#endif
Composition fixture(int pitch=60,int channel=16){
    Composition c;c.id="composer-player";c.endTick={1920};c.tempos={{{0},500000}};c.meters={{{0},4,4}};c.keys={{{0},0,Scale::major}};
    Track t;t.id=1;t.name="Track";t.channel=channel;t.program=33;t.bank=Bank{2,7};t.notes={{2,pitch,90,{0},{960}}};t.controls={{3,{0},1,70},{4,{1920},1,0}};c.tracks={t};return c;
}
PreparedSong ready(const Composition& c){auto p=prepareComposition(c);check(p.song);return std::move(*p.song);}
ComposerPlayerResult tracked(ComposerPlayer& p,const PlaybackBlock& b,juce::MidiBuffer& midi){
    allocationGuard::allocations=allocationGuard::frees=0;allocationGuard::active=true;auto result=p.process(b,midi);allocationGuard::active=false;
    check(allocationGuard::allocations==0&&allocationGuard::frees==0&&!result.outputRejected);check(midi.getNumEvents()==result.playback.messages&&midi.data.size()<=18432);
    check(allPacketsSurviveVst3(midi));return result;
}
int count(const juce::MidiBuffer& midi,bool on,int pitch=-1){int n=0;for(auto e:midi){auto m=e.getMessage();if((on?m.isNoteOn():m.isNoteOff())&&(pitch<0||m.getNoteNumber()==pitch))++n;}return n;}
int main(){
    // Independent pinned JUCE encoder is the oracle for ordered short-packet bytes.
    {
        juce::MidiBuffer actual,expected;actual.ensureSize(32768);expected.ensureSize(32768);PreparedMidiSink sink(actual);
        allocationGuard::allocations=allocationGuard::frees=0;allocationGuard::active=true;
        for(int ch=0;ch<16;++ch)for(int kind=0;kind<6;++kind){const uint8_t statuses[]={0x80,0x90,0xb0,0xc0,0xd0,0xe0};MidiPacket p{uint8_t(statuses[kind]|ch),uint8_t(ch*7),uint8_t(kind*23),uint8_t(kind==3||kind==4?2:3)};check(sink(p,ch*8+kind/2));}
        allocationGuard::active=false;check(allocationGuard::allocations==0&&allocationGuard::frees==0&&!sink.rejected());
        for(int ch=0;ch<16;++ch)for(int kind=0;kind<6;++kind){const uint8_t statuses[]={0x80,0x90,0xb0,0xc0,0xd0,0xe0};const uint8_t p[]={uint8_t(statuses[kind]|ch),uint8_t(ch*7),uint8_t(kind*23)};expected.addEvent(p,kind==3||kind==4?2:3,ch*8+kind/2);}
        check(actual.data==expected.data);
        actual.clear();expected.clear();PreparedMidiSink full(actual);allocationGuard::active=true;
        for(int i=0;i<2048;++i)check(full({uint8_t(0xb0|(i%16)),1,uint8_t(i%128),3},i/4));
        allocationGuard::active=false;check(allocationGuard::allocations==0&&allocationGuard::frees==0);
        for(int i=0;i<2048;++i){const uint8_t p[]={uint8_t(0xb0|(i%16)),1,uint8_t(i%128)};expected.addEvent(p,3,i/4);}
        check(actual.data==expected.data&&actual.data.size()==18432);auto size=actual.data.size();check(!full({0x90,60,90,3},1024)&&full.rejected()&&actual.data.size()==size);
        juce::MidiBuffer invalid;invalid.ensureSize(32768);PreparedMidiSink unordered(invalid);check(unordered({0x90,60,90,3},5)&&!unordered({0x80,60,0,3},4)&&unordered.rejected());
        invalid.clear();PreparedMidiSink badLength(invalid);check(!badLength({0x90,60,90,255},0)&&invalid.isEmpty()&&badLength.rejected());
    }
    ComposerPlayer player;PlaybackBlock block;block.samples=32;juce::MidiBuffer midi;midi.ensureSize(32768);
    midi.addEvent(juce::MidiMessage::noteOn(2,72,(juce::uint8)100),0);auto r=tracked(player,block,midi);check(!r.hasSong&&midi.isEmpty());
    auto one=ready(fixture());check(player.publish(std::move(one),1)==PublishStatus::published);r=tracked(player,block,midi);check(r.hasSong&&r.tag==1&&r.playback.messages==5&&count(midi,true,60)==1);
    int index=0;for(auto e:midi){auto m=e.getMessage();check(m.getChannel()==16);if(index==0)check(m.isController()&&m.getControllerNumber()==0&&m.getControllerValue()==2);if(index==1)check(m.isController()&&m.getControllerNumber()==32&&m.getControllerValue()==7);if(index==2)check(m.isProgramChange()&&m.getProgramChangeNumber()==33);++index;}
    auto two=ready(fixture(64,1));check(player.publish(std::move(two),2)==PublishStatus::published);block.position=r.playback.nextPosition;block.samples=0;r=tracked(player,block,midi);check(r.hasSong&&r.tag==1&&midi.isEmpty());
    block.samples=64;r=tracked(player,block,midi);check(r.tag==2&&count(midi,false,60)==1&&count(midi,true,64)==1);auto first=(*midi.begin()).getMessage();check(first.isNoteOff()&&first.getChannel()==16);
    player.reclaim();player.clear();r=tracked(player,block,midi);check(!r.hasSong&&r.playback.messages==4&&count(midi,false,64)==1);r=tracked(player,block,midi);check(r.playback.messages==0);player.reclaim();check(player.retainedBytes()==0);
    // Latest replacement before acquisition and stop/end semantics.
    auto three=ready(fixture(65)),four=ready(fixture(67));check(player.publish(std::move(three),3)==PublishStatus::published);check(player.publish(std::move(four),4)==PublishStatus::published);
    block.position=0;block.playing=true;r=tracked(player,block,midi);check(r.tag==4&&count(midi,true,65)==0&&count(midi,true,67)==1);block.playing=false;r=tracked(player,block,midi);check(count(midi,false,67)==1);
    block.playing=true;block.position=0;block.samples=96001;r=tracked(player,block,midi);check(r.playback.ended&&count(midi,true,67)==1&&count(midi,false,67)==1);bool finalCC=false;for(auto e:midi){auto m=e.getMessage();if(m.isController()&&m.getControllerNumber()==1&&m.getControllerValue()==0&&e.samplePosition==96000)finalCC=true;}check(finalCC);
    auto maximum=fixture();maximum.tracks.clear();for(int ch=1;ch<=16;++ch){Track t;t.id=Id(ch);t.name="Channel";t.channel=ch;t.role=ch==10?Role::drums:Role::bass;t.program=3;t.bank=Bank{1,2};for(int i=0;i<16;++i){t.notes.push_back({Id(1000+ch*16+i),i,90,{0},{480}});t.controls.push_back({Id(3000+ch*16+i),{0},i+1,64});}maximum.tracks.push_back(t);}
    auto max=ready(maximum);check(player.publish(std::move(max),5)==PublishStatus::published);block.position=0;block.samples=32;r=tracked(player,block,midi);check(r.playback.messages==560&&count(midi,true)==256);block.playing=false;r=tracked(player,block,midi);check(r.playback.messages==304&&count(midi,false)==256);
    // Fresh preallocated caller buffers, changing tempo and cold seek at32/64samples.
    for(int samples:{32,64})for(int i=0;i<8;++i){juce::MidiBuffer fresh;fresh.ensureSize(32768);block.samples=samples;block.playing=i!=3;block.bpm=i%2?90:170;block.position=i==6?0:double(i)/100;r=tracked(player,block,fresh);check(!r.playback.invalidClock&&!r.playback.overflow);}
    auto dense=fixture();dense.endTick={131073};dense.tracks[0].notes.clear();dense.tracks[0].controls.clear();for(size_t i=0;i<131072;++i)dense.tracks[0].notes.push_back({i+10,60,90,{0},{int64_t(i+1)}});auto huge=ready(dense);check(player.publish(std::move(huge),6)==PublishStatus::published);block.position=0;block.playing=true;block.samples=10000000;r=tracked(player,block,midi);check(r.playback.overflow&&count(midi,true,60)==count(midi,false,60));block.position=r.playback.nextPosition;r=tracked(player,block,midi);check(r.playback.overflow&&midi.isEmpty());
    auto crowded=fixture();crowded.endTick={7680};crowded.tracks[0].notes={{2,60,90,{0},{7680}}};crowded.tracks[0].controls.clear();for(int i=1;i<=2000;++i)crowded.tracks[0].controls.push_back({Id(10000+i),{i},1,i%128});auto full=ready(crowded);check(player.publish(std::move(full),7)==PublishStatus::published);block.position=0;block.samples=200000;r=tracked(player,block,midi);check(r.playback.overflow&&r.playback.messages>1600&&count(midi,true)==count(midi,false));
    player.clear();tracked(player,block,midi);player.reclaim();player.resetStopped();check(player.retainedBytes()==0);
    // Concurrent producer reclamation must not free storage in the actual JUCE callback.
    ComposerPlayer concurrent;std::atomic<bool> done=false;std::atomic<int> callbacks=0;int held[2048]{};
    std::thread producer([&]{while(callbacks.load(std::memory_order_acquire)==0)std::this_thread::yield();for(uint64_t tag=1;tag<=2048;++tag){auto next=ready(fixture(int(tag%96)+16));PublishStatus status;do{status=concurrent.publish(std::move(next),tag);if(status==PublishStatus::busy)std::this_thread::yield();}while(status==PublishStatus::busy);check(status==PublishStatus::published);if(tag%32==0){int n=callbacks.load(std::memory_order_acquire);while(callbacks.load(std::memory_order_acquire)==n)std::this_thread::yield();}}done.store(true,std::memory_order_release);});
    block.samples=32;block.position=0;block.bpm=120;block.playing=true;
    auto consume=[&]{auto result=tracked(concurrent,block,midi);for(auto e:midi){auto m=e.getMessage();if(m.isNoteOn()){int key=(m.getChannel()-1)*128+m.getNoteNumber();check(held[key]++==0&&m.getNoteNumber()==int(result.tag%96)+16);}if(m.isNoteOff()){int key=(m.getChannel()-1)*128+m.getNoteNumber();check(--held[key]==0);}}callbacks.fetch_add(1,std::memory_order_release);return result;};
    while(!done.load(std::memory_order_acquire))consume();
    producer.join();block.playing=false;r=consume();check(r.tag==2048);
    for(int n:held)check(n==0);
    concurrent.clear();consume();concurrent.reclaim();concurrent.resetStopped();
    std::cout<<"JUCE prepared MIDI callback: slots/clear/zero/32-64/tempo/overflow, full-channel presets/CC and VST3 conversion; no callback allocations/frees passed\n";
}
