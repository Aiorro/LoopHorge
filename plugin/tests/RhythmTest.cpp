#include "../Source/engine/Rhythm.h"
#include "../Source/engine/Generation.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>

using namespace composer;
#define check(x) do {if(!(x)){std::cerr<<"line "<<__LINE__<<": " #x "\n";std::abort();}}while(false)
uint64_t signature(const Track& track){
    uint64_t hash=14695981039346656037ULL;
    auto add=[&](int64_t value){for(int i=0;i<8;++i){hash^=uint64_t(value)>>(i*8)&255;hash*=1099511628211ULL;}};
    add(int64_t(track.id));add(track.channel);
    for(const auto& n:track.notes){add(int64_t(n.id));add(n.pitch);add(n.velocity);add(n.start.value);add(n.duration.value);}
    return hash;
}
void bounds(const Config& cfg,const Track& track){
    check(track.channel==10&&track.role==Role::drums&&!track.program&&!track.bank&&track.controls.empty());
    check(!track.notes.empty());
    std::array<int64_t,128> finish{};
    int64_t barTicks=480LL*4*cfg.numerator/cfg.denominator,lastStart=-1;
    for(const auto& n:track.notes){
        check(n.pitch>=35&&n.pitch<=59&&track.ownedPitches.test(size_t(n.pitch)));
        check(n.velocity>=1&&n.velocity<=127&&n.start.value>=0&&n.duration.value>0);
        check(n.start.value>=lastStart&&n.start.value>=finish[size_t(n.pitch)]);
        check(n.start.value+n.duration.value<=cfg.bars*barTicks);
        check(n.start.value%barTicks+n.duration.value<=barTicks);
        finish[size_t(n.pitch)]=n.start.value+n.duration.value;lastStart=n.start.value;
    }
}
int main(){
    Config cfg;cfg.seed=42;cfg.drums=true;cfg.humanization=0;
    auto r=generateDrums(cfg);check(r.track&&r.error.empty());bounds(cfg,*r.track);
    check(signature(*generateDrums(cfg).track)==signature(*r.track));
    std::set<uint64_t> seeds;for(uint32_t i=0;i<16;++i){cfg.seed=i;seeds.insert(signature(*generateDrums(cfg).track));}check(seeds.size()>1);
    std::set<std::vector<std::pair<int64_t,int>>> styles;
    for(int style=0;style<5;++style){cfg.style=Style(style);r=generateDrums(cfg);check(r.track);bounds(cfg,*r.track);
        std::vector<std::pair<int64_t,int>> events;
        for(const auto& n:r.track->notes)if(n.start.value<1920)events.emplace_back(n.start.value,n.pitch);
        styles.insert(events);
    }check(styles.size()==5);
    cfg=Config{};cfg.drums=true;cfg.style=Style::boomBap;cfg.humanization=0;
    auto withGhosts=*generateDrums(cfg).track;cfg.ghostNotes=false;auto withoutGhosts=*generateDrums(cfg).track;
    bool ghost=false;for(const auto& n:withGhosts.notes)if(n.pitch==38&&n.velocity<=42)ghost=true;check(ghost);
    for(const auto& n:withoutGhosts.notes)if(n.pitch==38)check(n.velocity>42);
    check(withGhosts.notes.size()>withoutGhosts.notes.size());
    cfg.ghostNotes=true;auto withFill=*generateDrums(cfg).track;cfg.drumFills=false;auto noFill=*generateDrums(cfg).track;
    bool tom=false;for(const auto& n:withFill.notes)if(n.pitch==45||n.pitch==47||n.pitch==50){tom=true;check((n.start.value/1920+1)%4==0);}check(tom);
    for(const auto& n:noFill.notes)check(n.pitch!=45&&n.pitch!=47&&n.pitch!=50);

    cfg=Config{};cfg.seed=42;auto melodic=*generateLoop(cfg).composition;cfg.drums=true;
    auto full=generateLoop(cfg);check(full.composition&&validate(*full.composition));check(full.composition->tracks.size()==4);
    for(size_t i=0;i<3;++i)check(signature(melodic.tracks[i])==signature(full.composition->tracks[i]));
    bounds(cfg,full.composition->tracks[3]);
    for(int numerator:{1,3,6,12})for(int denominator:{2,4,8,16})for(int style=0;style<5;++style)for(uint32_t seed=0;seed<16;++seed){
        cfg.numerator=numerator;cfg.denominator=denominator;cfg.style=Style(style);cfg.seed=seed;
        cfg.humanization=100;cfg.swing=100;cfg.complexity=100;
        r=generateDrums(cfg);check(r.track);bounds(cfg,*r.track);
        auto song=generateLoop(cfg);check(song.composition&&validate(*song.composition));
    }
    auto snareOnsets=[](const Track& track){std::vector<int64_t> out;for(const auto& n:track.notes)if(n.pitch==38&&n.start.value<1920)out.push_back(n.start.value);return out;};
    cfg=Config{};cfg.drums=true;cfg.style=Style::boomBap;cfg.humanization=0;cfg.ghostNotes=false;cfg.drumFills=false;
    cfg.feel=Feel::normal;check(snareOnsets(*generateDrums(cfg).track)==std::vector<int64_t>({480,1440}));
    cfg.feel=Feel::half;check(snareOnsets(*generateDrums(cfg).track)==std::vector<int64_t>({960}));
    cfg.feel=Feel::doubleTime;check(snareOnsets(*generateDrums(cfg).track)==std::vector<int64_t>({240,720,1200,1680}));
    auto even=musicDetail::rhythmGrid({0,1,2,3,4},5,Feel::half,0);
    auto odd=musicDetail::rhythmGrid({0,1,2,3,4},5,Feel::half,1);
    check(even.divisions==5&&even.pulses.size()==3&&odd.pulses.size()==2);
    check(even.pulses[0].unit==0&&even.pulses[1].unit==2&&even.pulses[2].unit==4);
    check(odd.pulses[0].unit==1&&odd.pulses[1].unit==3);
    check(odd.pulses[0].sourceIndex==3&&odd.pulses[1].sourceIndex==4);
    auto twice=musicDetail::rhythmGrid({0,1,2,3,4},5,Feel::doubleTime,0);
    check(twice.divisions==10&&twice.pulses.size()==10&&twice.pulses[5].unit==5&&twice.pulses[5].sourceIndex==0&&twice.pulses[5].cycle==1);
    cfg=Config{};cfg.drums=true;cfg.humanization=0;
    auto normal=*generateLoop(cfg).composition;cfg.feel=Feel::half;auto half=*generateLoop(cfg).composition;
    cfg.feel=Feel::doubleTime;auto doubled=*generateLoop(cfg).composition;
    for(auto* song:{&half,&doubled}){check(validate(*song));check(song->endTick==normal.endTick&&song->tempos[0].microsPerQuarter==normal.tempos[0].microsPerQuarter);check(song->id!=normal.id);}
    for(size_t track:{size_t(0),size_t(2)}){check(half.tracks[track].notes.size()*2==normal.tracks[track].notes.size());check(doubled.tracks[track].notes.size()==normal.tracks[track].notes.size()*2);}
    for(auto feel:{Feel::half,Feel::normal,Feel::doubleTime}){cfg.feel=feel;cfg.drums=false;auto before=*generateLoop(cfg).composition;cfg.drums=true;auto after=*generateLoop(cfg).composition;
        for(size_t i=0;i<3;++i)check(signature(before.tracks[i])==signature(after.tracks[i]));}
    cfg=Config{};cfg.style=Style::trap;cfg.humanization=0;cfg.swing=100;cfg.ghostNotes=false;cfg.drumFills=false;
    cfg.feel=Feel::half;r=generateDrums(cfg);check(r.track);bool halfSwing=false;
    for(const auto& n:r.track->notes){if(n.pitch==42&&n.start.value==720)halfSwing=true;}check(halfSwing);
    cfg.feel=Feel::doubleTime;r=generateDrums(cfg);bool firstSwing=false,secondSwing=false;
    for(const auto& n:r.track->notes){if(n.pitch==42){if(n.start.value==180)firstSwing=true;if(n.start.value==1140)secondSwing=true;}}check(firstSwing&&secondSwing);
    for(auto feel:{Feel::half,Feel::normal,Feel::doubleTime})for(int numerator:{1,6,12})for(int denominator:{2,8,16})for(int style=0;style<5;++style)for(uint32_t seed=0;seed<16;++seed){
        cfg=Config{};cfg.drums=true;cfg.feel=feel;cfg.numerator=numerator;cfg.denominator=denominator;cfg.style=Style(style);cfg.seed=seed;
        cfg.humanization=100;cfg.swing=100;cfg.complexity=100;r=generateDrums(cfg);check(r.track);bounds(cfg,*r.track);
        auto song=generateLoop(cfg);check(song.composition&&validate(*song.composition));
        for(size_t part:{size_t(0),size_t(2)}){const auto& notes=song.composition->tracks[part].notes;for(size_t i=1;i<notes.size();++i)check(notes[i].start>notes[i-1].start);}
    }
    for(int complexity:{0,24})for(int style=0;style<5;++style)for(auto feel:{Feel::half,Feel::normal,Feel::doubleTime}){
        Config selected;selected.drums=true;selected.complexity=complexity;selected.style=Style(style);selected.feel=feel;selected.humanization=0;selected.drumFills=false;
        auto enabled=generateDrums(selected);check(enabled.track);bool actualGhost=false;const int snare=style==int(Style::ambient)?37:38;
        for(const auto& n:enabled.track->notes){if(n.pitch==snare&&n.velocity<=42)actualGhost=true;}check(actualGhost);
        selected.ghostNotes=false;auto disabled=generateDrums(selected);check(disabled.track);
        for(const auto& n:disabled.track->notes){if(n.pitch==snare)check(n.velocity>42);}
        check(enabled.track->notes.size()>disabled.track->notes.size());
    }
    cfg=Config{};cfg.seed=std::numeric_limits<uint32_t>::max();cfg.root=11;cfg.scale=Scale::blues;cfg.style=Style::ambient;cfg.bpm=40;cfg.bars=32;
    cfg.numerator=12;cfg.denominator=2;cfg.complexity=100;cfg.humanization=100;cfg.swing=100;cfg.harmonicBars=4;cfg.modalInterchange=true;
    cfg.drums=true;cfg.feel=Feel::doubleTime;cfg.modulation=Modulation{28,5,Scale::harmonicMinor};
    check(generateLoop(cfg).composition);
    cfg.feel=static_cast<Feel>(999);check(!generateLoop(cfg).composition&&!generateDrums(cfg).track);cfg.feel=Feel::normal;
    cfg.numerator=0;r=generateDrums(cfg);check(!r.track&&!r.error.empty());
    std::cout<<"native drums: GM10,5grooves,ghosts/fills,seed independence and3440boundary configurations passed\n";
}
