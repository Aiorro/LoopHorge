#include "../Source/engine/Rhythm.h"
#include "../Source/engine/Generation.h"
#include <array>
#include <cstdlib>
#include <iostream>
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
    cfg.numerator=0;r=generateDrums(cfg);check(!r.track&&!r.error.empty());
    std::cout<<"native drums: GM10,5grooves,ghosts/fills,seed independence and1280boundary configurations passed\n";
}
