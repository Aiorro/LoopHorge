#include "../Source/ComposerPlayer.h"
#include <chrono>
#include <iostream>

// Explicit local measurement, not a timing assertion on shared CI machines.
int main(){
    using namespace composer;Composition c;c.id="timing";c.endTick={1920};c.tempos={{{0},500000}};c.meters={{{0},4,4}};c.keys={{{0},0,Scale::major}};
    for(int ch=1;ch<=16;++ch){Track t;t.id=Id(ch);t.name="Track";t.channel=ch;t.role=ch==10?Role::drums:Role::bass;t.program=3;t.bank=Bank{1,2};
        for(int i=0;i<16;++i){t.notes.push_back({Id(1000+ch*16+i),i,90,{0},{480}});t.controls.push_back({Id(3000+ch*16+i),{0},i+1,64});}c.tracks.push_back(t);}
    auto song=prepareComposition(c);if(!song.song)return 1;ComposerPlayer player;if(player.publish(std::move(*song.song),1)!=PublishStatus::published)return 2;
    juce::MidiBuffer midi;midi.ensureSize(32768);PlaybackBlock block;block.samples=32;
    std::array<double,500> times{};
    for(size_t i=0;i<times.size();++i){auto start=std::chrono::steady_clock::now();auto r=player.process(block,midi);times[i]=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
        if(r.outputRejected||!r.hasSong||r.playback.messages!=(i==0?560:864)||midi.getNumEvents()!=r.playback.messages)return 3;}
    std::sort(times.begin(),times.end());
    std::cout<<"256-key seek,32samples: median "<<times[250]<<"us, p95 "<<times[475]<<"us, max "<<times.back()<<"us;48kHz block666.7us (local observation, not a real-time guarantee)\n";
}
