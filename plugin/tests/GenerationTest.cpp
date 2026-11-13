#include "../Source/engine/Generation.h"
#include "../Source/engine/Time.h"
#include <cstdlib>
#include <iostream>
#include <set>

using namespace composer;
#define check(x) do {if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
uint64_t signature(const Composition& c){
    uint64_t hash=14695981039346656037ULL;
    auto add=[&](int64_t value){for(int i=0;i<8;++i){hash^=uint64_t(value)>>(i*8)&255;hash*=1099511628211ULL;}};
    add(c.seed);add(c.endTick.value);
    for(const auto& chord:c.chords){add(chord.start.value);add(chord.duration.value);add(chord.root);for(int p:chord.voicing)add(p);}
    for(const auto& track:c.tracks){add(track.channel);for(const auto& n:track.notes){add(n.pitch);add(n.velocity);add(n.start.value);add(n.duration.value);}}
    return hash;
}
const Chord& chordAt(const Composition& c,int64_t tick){for(const auto& h:c.chords)if(tick>=h.start.value&&tick<h.start.value+h.duration.value)return h;std::abort();}
bool chordTone(const Chord& h,int pitch){return std::find(h.pitchClasses.begin(),h.pitchClasses.end(),pitch%12)!=h.pitchClasses.end();}
int main(){
    Config cfg;cfg.seed=42;cfg.humanization=0;
    auto generated=generateLoop(cfg);check(generated.composition&&generated.error.empty());auto c=*generated.composition;
    check(validate(c));check(c.tracks.size()==3&&c.chords.size()==8&&c.endTick.value==15360);
    check(c.seed==42&&c.tempos[0].microsPerQuarter==600000);
    auto original=signature(c);check(original==7194293116935115149ULL);check(signature(*generateLoop(cfg).composition)==original);
    ++cfg.seed;check(signature(*generateLoop(cfg).composition)!=original);cfg.seed=42;
    for(const auto& n:c.tracks[0].notes)check(chordTone(chordAt(c,n.start.value),n.pitch));
    for(const auto& n:c.tracks[2].notes)if(n.start.value%1920==0)check(chordTone(chordAt(c,n.start.value),n.pitch));
    std::vector<int> first,repeated;
    for(const auto& n:c.tracks[2].notes){if(n.start.value<1920)first.push_back(n.pitch);if(n.start.value>=7680&&n.start.value<9600)repeated.push_back(n.pitch);}
    check(first==repeated&&!first.empty());
    std::set<std::vector<int64_t>> rhythms;
    for(int style=0;style<5;++style){cfg.style=Style(style);auto song=generateLoop(cfg);check(song.composition);std::vector<int64_t> onsets;
        for(const auto& n:song.composition->tracks[0].notes)if(n.start.value<1920)onsets.push_back(n.start.value);
        rhythms.insert(onsets);
    }check(rhythms.size()==5);
    for(int root=0;root<12;++root)for(int scale=0;scale<=int(Scale::blues);++scale)for(int style=0;style<5;++style){
        cfg.root=root;cfg.scale=Scale(scale);cfg.style=Style(style);auto song=generateLoop(cfg);
        if(!song.composition)std::cerr<<"root="<<root<<" scale="<<scale<<" style="<<style<<" "<<song.error<<"\n";
        check(song.composition&&validate(*song.composition));
        auto intervals=scaleIntervals(cfg.scale);
        for(const auto& track:song.composition->tracks)for(const auto& n:track.notes){
            int relative=(n.pitch-root+120)%12;check(std::find(intervals.begin(),intervals.end(),relative)!=intervals.end());
            check(n.pitch>=track.lowPitch&&n.pitch<=track.highPitch);
        }
    }
    cfg=Config{};cfg.bars=16;cfg.numerator=6;cfg.denominator=8;cfg.humanization=100;cfg.swing=100;cfg.complexity=100;
    c=*generateLoop(cfg).composition;check(validate(c));check(c.endTick.value==23040);
    for(const auto& t:c.tracks)for(const auto& n:t.notes)check(n.start.value>=0&&n.start.value+n.duration.value<=c.endTick.value);
    for(size_t i=1;i<c.tracks[2].notes.size();++i)check(std::abs(c.tracks[2].notes[i].pitch-c.tracks[2].notes[i-1].pitch)<=12);
    cfg=Config{};cfg.bars=32;cfg.numerator=3;cfg.denominator=2;cfg.bpm=64;
    c=*generateLoop(cfg).composition;check(std::abs(secondsAt(c,c.endTick)->value-180.0)<1e-9);
    for(int seed=0;seed<200;++seed){
        cfg=Config{};cfg.seed=uint32_t(seed);cfg.numerator=1;cfg.denominator=16;cfg.complexity=100;cfg.swing=100;cfg.humanization=100;
        auto edge=generateLoop(cfg);check(edge.composition);
        const auto& notes=edge.composition->tracks[2].notes;
        for(size_t i=1;i<notes.size();++i){if(notes[i].start<=notes[i-1].start)std::cerr<<"humanization seed="<<seed<<" tick="<<notes[i].start.value<<"\n";check(notes[i].start>notes[i-1].start);}
    }
    cfg=Config{};cfg.modalInterchange=true;c=*generateLoop(cfg).composition;check(validate(c));
    bool borrowed=false;for(const auto& h:c.chords)if(!h.explanation.empty())borrowed=true;check(borrowed);
    for(const auto& n:c.tracks[0].notes)check(chordTone(chordAt(c,n.start.value),n.pitch));
    for(int harmonicBars:{1,2,4}){auto requested=cfg;requested.harmonicBars=harmonicBars;auto song=generateLoop(requested);check(song.composition);
        bool applied=false;for(const auto& h:song.composition->chords)if(!h.explanation.empty())applied=true;check(applied);}
    cfg.modulation=Modulation{4,7,Scale::major};c=*generateLoop(cfg).composition;
    check(validate(c)&&c.keys.size()==2&&c.keys[1].tick.value==7680&&c.keys[1].root==7);
    for(const auto& n:c.tracks[0].notes)check(chordTone(chordAt(c,n.start.value),n.pitch));
    cfg=Config{};cfg.harmonicBars=2;auto harmony=planHarmony(cfg);check(harmony.error.empty()&&harmony.chords.size()==4);
    for(const auto& h:harmony.chords)check(h.duration.value==3840);
    auto rejects=[](auto mutate){Config bad;mutate(bad);auto r=generateLoop(bad);check(!r.composition&&!r.error.empty());};
    rejects([](auto& x){x.bpm=0;});rejects([](auto& x){x.bars=9999999;});rejects([](auto& x){x.root=12;});
    rejects([](auto& x){x.scale=Scale(999);});rejects([](auto& x){x.style=Style(999);});
    rejects([](auto& x){x.numerator=0;});rejects([](auto& x){x.denominator=3;});
    rejects([](auto& x){x.complexity=101;});rejects([](auto& x){x.humanization=-1;});rejects([](auto& x){x.swing=101;});
    rejects([](auto& x){x.harmonicBars=3;});rejects([](auto& x){x.bars=4;x.harmonicBars=4;x.modalInterchange=true;});rejects([](auto& x){x.modulation=Modulation{3,4,Scale::major};});
    std::cout<<"native generation: shared harmony, motifs,660configurations, groove, seed and bounds passed; golden="<<original<<"\n";
}
