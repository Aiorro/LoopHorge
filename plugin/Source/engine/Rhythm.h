#pragma once
#include "Config.h"

namespace composer {
namespace musicDetail {
struct Pulse {int unit=0;size_t sourceIndex=0;int cycle=0;};
struct Grid {std::vector<Pulse> pulses;int divisions=0,stepMultiplier=1;};
// Internal inputs are the fixed, ordered profiles below and the validated Config bar/feel.
inline Grid rhythmGrid(const std::vector<int>& units,int divisions,Feel feel,int bar){
    Grid grid;grid.divisions=feel==Feel::doubleTime?divisions*2:divisions;
    grid.stepMultiplier=feel==Feel::half?2:1;
    for(int cycle=0;cycle<(feel==Feel::doubleTime?2:1);++cycle)for(size_t i=0;i<units.size();++i){
        int unit=units[i]+cycle*divisions;
        if(feel==Feel::half){unit=units[i]*2-(bar%2)*divisions;if(unit<0||unit>=divisions)continue;}
        grid.pulses.push_back({unit,i,cycle});
    }
    return grid;
}
inline int64_t onset(int unit,int divisions,int64_t length,const Config& config,Random& random,int originalUnit=-1,int stepMultiplier=1){
    const int64_t base=int64_t(unit)*length/divisions;
    if(originalUnit<0)originalUnit=unit;
    if(unit==0||originalUnit==0)return base;
    const int64_t step=std::max<int64_t>(1,length*stepMultiplier/divisions);
    const int64_t swing=originalUnit%2?step*config.swing/200:0;
    const int jitter=int(std::min<int64_t>((step-1)/4,24)*config.humanization/100);
    return std::clamp(base+swing+random.offset(jitter),int64_t(0),length-1);
}
}
struct DrumResult {std::optional<Track> track;std::string error;};

inline DrumResult generateDrums(const Config& config){
    auto checked=validateConfig(config);if(!checked)return {{},checked.message};
    try {
        Track track;track.id=1;track.name="Drums";track.role=Role::drums;track.channel=10;track.lowPitch=35;track.highPitch=59;
        for(size_t pitch=35;pitch<=59;++pitch)track.ownedPitches.set(pitch);
        const int64_t barTicks=480LL*4*config.numerator/config.denominator;
        const std::vector<int> kicks[]={{0,6,11},{0,7,10},{0,5,10,14},{0},{0,2,6,8,11,14}};
        const std::vector<int> snares[]={{8},{4,12},{4,12},{8},{4,12}};
        const std::vector<int> hats[]={{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},{0,2,4,6,8,10,12,14},{0,2,4,6,8,10,12,14},{0,8},{0,2,4,6,8,10,12,14}};
        const std::vector<int> ghosts[]={{7,15},{3,11},{6,10,15},{6},{7,15}};
        const int style=int(config.style);
        for(int bar=0;bar<config.bars;++bar){
            auto add=[&](int pitch,int64_t tick,int velocity){
                const int64_t base=std::min(int64_t(60),std::max<int64_t>(1,barTicks/32));
                const int64_t duration=std::clamp(base*config.performance.gatePercent/100,int64_t(1),barTicks-tick);
                track.notes.push_back({0,pitch,std::clamp(velocity,1,127),{bar*barTicks+tick},{duration}});
            };
            auto voice=[&](const std::vector<int>& units,int pitch,int velocity,uint32_t role,bool accented){
                musicDetail::Random random(musicDetail::seedFor(config.seed,role,uint32_t(bar)));
                auto grid=musicDetail::rhythmGrid(units,16,config.feel,bar);
                for(const auto& pulse:grid.pulses){
                    if(role==102&&pulse.sourceIndex%size_t(config.performance.bassStride)!=0&&pulse.sourceIndex+1!=units.size())continue;
                    int original=units[pulse.sourceIndex];
                    int64_t tick=musicDetail::onset(pulse.unit,grid.divisions,barTicks,config,random,original,grid.stepMultiplier);
                    int strength=velocity+(accented&&original%4==0?12:0)+random.offset(config.humanization/10);
                    strength+=role==103?config.performance.velocityOffset/3:config.performance.velocityOffset;
                    if(role==103)strength=std::clamp(strength,1,42);
                    add(pitch,tick,strength);
                }
            };
            const bool ambient=config.style==Style::ambient;
            voice(kicks[style],36,ambient?62:104,100,true);
            voice(snares[style],ambient?37:38,ambient?56:108,101,false);
            voice(hats[style],ambient?51:42,ambient?40:58,102,true);
            if(config.ghostNotes)voice(ghosts[style],ambient?37:38,ambient?20:30,103,false);
            if(config.drumFills&&(bar+1)%4==0){
                musicDetail::Random random(musicDetail::seedFor(config.seed,104,uint32_t(bar)));
                int count=ambient?2:config.complexity>=75?8:4;
                if(config.feel==Feel::half)count=std::max(1,count/2);
                if(config.feel==Feel::doubleTime)count*=2;
                const int toms[]{45,47,50};const int rotation=int(random.next()%3);
                const int64_t begin=barTicks*3/4,length=barTicks-begin;
                for(int i=0;i<count;++i){
                    const int64_t tick=begin+int64_t(i)*length/count;
                    const int pitch=config.style==Style::trap?38:toms[(rotation+i)%3];
                    add(pitch,tick,(ambient?42:84)+i*12/count+random.offset(config.humanization/10)+config.performance.velocityOffset);
                }
            }
        }
        std::sort(track.notes.begin(),track.notes.end(),[](const auto& a,const auto& b){return a.start<b.start||(a.start==b.start&&a.pitch<b.pitch);});
        std::vector<Note> canonical;canonical.reserve(track.notes.size());
        for(const auto& note:track.notes){
            if(!canonical.empty()&&canonical.back().start==note.start&&canonical.back().pitch==note.pitch){
                canonical.back().velocity=std::max(canonical.back().velocity,note.velocity);
                canonical.back().duration=std::max(canonical.back().duration,note.duration);
            }else canonical.push_back(note);
        }
        std::array<size_t,128> previous;previous.fill(canonical.size());
        for(size_t i=0;i<canonical.size();++i){
            auto& note=canonical[i];auto& prior=previous[size_t(note.pitch)];
            if(prior<canonical.size())canonical[prior].duration.value=std::min(canonical[prior].duration.value,note.start.value-canonical[prior].start.value);
            prior=i;note.id=uint64_t(i)+2;
        }
        track.notes=std::move(canonical);
        return {std::move(track),{}};
    }catch(const std::bad_alloc&){return {{},"Not enough memory to generate drums"};}
}
}
